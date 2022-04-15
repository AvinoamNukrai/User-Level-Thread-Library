// Written by Avinoam Nukrai

// Imports
#include <iostream>
#include "uthreads.h"
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include "queue"
#include "unordered_map"
#include <deque>
#include<list>
#include <algorithm>
#include "Thread.h"

// Constants, Globals and Typedefs
#define SECOND 1000000
#define MAIN_THREAD_ID 0
#define FAILURE -1
#define PROGRAM_SIGNAL 3
#define SYSTEM_ERROR(info) "system error: " << (info) << endl
#define LIB_ERROR(info) "thread library error: " << (info) << endl
#define FAILED_BLOCK_ALL_SIGNALS "Failed to blocked all signals"
#define FAILED_UNBLOCK_ALL_SIGNALS "Failed to unblock all signals"
#define INVALID_THREAD_ID "Such a thread is isn't exist"
#define SIGADDSET_ERROR "sigaddset function was failed"
#define SIGACTION_ERROR " sigaction function was failed, the time_handler was not succesfuly set"
#define EXCEED_THRADS_NUM "Exceed number of threads in the process"
#define BAD_ALLOC "Failed to allocate a new thread"
#define BLOCK_MAIN_THREAD "Cant block the main thread of the process"
#define INVALID_QUANTUM_NUM "Invalid number of quantums"
#define SLEEP_MAIN_THREAD "Cant put the main thread in sleep mode"
using namespace std;
typedef void (*thread_entry_point)(void);
static int process_quantum_counter = 1; // Counter of all quantum done in the process
struct itimerval timer; // Timer for being able to handle the quantum of all_threads
struct sigaction sig_action = {0}; // sig_action for defining a Handler of signals

// Data structures
Thread* main_thread; // The main thread in the process
Thread* running_thread; // The current running thread
unordered_map<int, Thread *> all_threads; // Map that contains all all_threads in the process (blocked, running, ready and sleeping)
list<int> blocked; // List that contains all the ID's of blocked all_threads in the process
deque<int> ready_threads; // Deque that contains all the ID's of the current ready all_threads in the process
unordered_map<int, int> sleep_threads; // Key - ID thread ; value - quantum remains
sigset_t blocked_sig_set; // sigest_t (set of signals) of all the currently blocked signals in the process
priority_queue<int, vector<int>, greater<int>> next_ID;


/**
 * This function block all the signals that suppose to jump to the process
 * @return int - indicates if the blocking was succeeded or not
 */
int block_all_signals(){
    if (sigprocmask(SIG_SETMASK, &blocked_sig_set, nullptr)){
        cerr << SYSTEM_ERROR(FAILED_BLOCK_ALL_SIGNALS);
        return FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * This function unblock all the signals that suppose to jump to the process
 * @return int - indicates if the unblocking was succeeded or not
 */
int unblock_all_signals(){
    if (sigprocmask(SIG_UNBLOCK, &blocked_sig_set, nullptr)){
        cerr << SYSTEM_ERROR(FAILED_UNBLOCK_ALL_SIGNALS);
        return FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * This function init and set the global timer to start over
 */
void init_timer(){
    if (setitimer(ITIMER_VIRTUAL, &timer, nullptr)){
        cerr << SYSTEM_ERROR("Failed to init the timer");
        exit(FAILURE);
    }
}

/**
 * This function is increment the total quantum for each thread in the process
 */
void inc_all_threads_total_quantum() {
    for (auto & thread : all_threads){
        thread.second->inc_thread_quantum_counter();
    }
}

/**
 * This functions is switching between the current running thread and the first thread that
 * is in the ready threads queue. Also updating the right destination of each thread in the right
 * data structure.
 */
void switch_threads() {
    if (block_all_signals()) exit(FAILURE);
    if (running_thread) { // if the running thread want to switch between it-self
        int return_value = sigsetjmp(running_thread->get_env(), 1);
        if (return_value) {
            if (unblock_all_signals()) exit(FAILURE);
            return;
        }
        ready_threads.push_back(running_thread->get_id());
    }
    // else, making real switch
    running_thread = all_threads[ready_threads.front()];
    ready_threads.pop_front();
    if (unblock_all_signals()) exit(FAILURE);
    init_timer();
    siglongjmp(running_thread->get_env(), 1); // jump to the updated thread
}

/**
 * This function updates the sleep counter of each thread in the process. the counter will be
 * decrees and if the sleep counter of a thread is reset and if the thread is not define as
 * blocked (mean that he is not in the blocked list) so the thread id will be
 * pushed into the ready threads queue and will be deleted from the sleeping threads map.
 */
void update_all_sleep_counters(){
    unordered_map<int, int> sleep_threads_to_delete;
    for (auto it = sleep_threads.begin(); it != sleep_threads.end(); it++) {
        it->second--;
        if (it->second == 0){
            if(std::count(blocked.begin(), blocked.end(), it->first) == 0){
                ready_threads.push_back(it->first);
            }
            sleep_threads_to_delete[it->first] = it->second;
        }
    }
    for (auto & it : sleep_threads_to_delete){
        sleep_threads.erase(it.first);
    }
}


/**
 * This function is the handler function of the algorithm define signals.
 * shes making the right switch of the threads and updates the timer and the counters, exactly as
 * algorithms Rabin-Rupin defined.
 * @param some_signal  - the signals that the cpu sends to the process.
 */
void time_handler(int some_signal){
    // check that we got the only algorithm defined signals
    if (some_signal == SIGVTALRM || some_signal == PROGRAM_SIGNAL){
        // block all the signals
        if (block_all_signals()) exit(FAILURE);
        // init the timer
        init_timer();
        // switch the all_threads (running_thread and all_threads[ready_queue.front()]
        switch_threads();
        // inc the right counters
        running_thread->inc_running_mode_counter();
        process_quantum_counter++;
        inc_all_threads_total_quantum();
        // decrement the sleep counter of each thread
        update_all_sleep_counters();
        // unblock all the signals
        if (unblock_all_signals()) exit(FAILURE);
    }
}


/**
 * @brief initializes the thread library.
 *
 * You may assume that this function is called before any other thread library function, and that it is called
 * exactly once.
 * The input to the function is the length of a quantum in micro-seconds.
 * It is an error to call this function with non-positive quantum_usecs.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs){
    // input validation
    if (quantum_usecs <= 0){
        cerr << SYSTEM_ERROR("failed to init the thread library");
        return FAILURE;
    }
    // creates the thread
    try {
        main_thread = new Thread(0, nullptr);
    }
    catch (bad_alloc &e) {
        cerr << SYSTEM_ERROR(BAD_ALLOC);
        return FAILURE;
    }
    // init the main thread to be the running one and insert him to the map
    all_threads[MAIN_THREAD_ID] = main_thread;
    running_thread = main_thread;
//    process_quantum_counter = 0;
    sigaddset(&sig_action.sa_mask, SIGVTALRM);
    sig_action.sa_handler = &time_handler; // set the time_handler to be the handler of all the valid signals
    if (sigaddset(&blocked_sig_set, SIGVTALRM)) {
        cerr << SYSTEM_ERROR(SIGADDSET_ERROR);
        exit(FAILURE);
    }
    // define the intervals in (sec and nano-sec) of the timer
    timer.it_value.tv_sec = (quantum_usecs / SECOND);
    timer.it_value.tv_usec = (quantum_usecs % SECOND);
    timer.it_interval.tv_sec = (quantum_usecs / SECOND);
    timer.it_interval.tv_usec = (quantum_usecs % SECOND);
    // push all the valid and possible id's in the process into a queue
    for (int i = 1; i < MAX_THREAD_NUM; ++i) {
        next_ID.push(i);
    }
    // define the handler of all signals in the library to be sig_action.sa_handler
    if (sigaction(SIGVTALRM, &sig_action, NULL) < 0){
        cerr << SYSTEM_ERROR(SIGACTION_ERROR);
        return FAILURE;
    }
    // saving the context of the main thread (needed by default)
    sigsetjmp(running_thread->get_env(), 1);
    init_timer();
    switch_threads(); // the actual start of running threads in the process
    return EXIT_SUCCESS;
}

/**
 * @brief Creates a new thread, whose entry point is the function entry_point with the signature
 * void entry_point(void).
 *
 * The thread is added to the end of the READY all_threads list.
 * The uthread_spawn function should fail if it would cause the number of concurrent all_threads to exceed the
 * limit (MAX_THREAD_NUM).
 * Each thread should be allocated with a stack of size STACK_SIZE bytes.
 *
 * @return On success, return the ID of the created thread. On failure, return -1.
*/
int uthread_spawn(thread_entry_point entry_point){
    if (block_all_signals()) return FAILURE;
    if (all_threads.size() >= MAX_THREAD_NUM){
        //Room for another Thread
        if (unblock_all_signals()) return FAILURE;
        cerr << LIB_ERROR(EXCEED_THRADS_NUM);
        return FAILURE;
    }
    //Creating new Thread
    Thread* new_thread;
    int new_thread_id = next_ID.top();
    next_ID.pop();
    try {
        new_thread = new Thread(new_thread_id, entry_point);
    }
    catch (bad_alloc& e){
        cerr << SYSTEM_ERROR(BAD_ALLOC);
        if (unblock_all_signals()) return FAILURE;
        return FAILURE;
    }
    all_threads[new_thread_id] = new_thread;
    ready_threads.push_back(new_thread_id);
    if (unblock_all_signals()) return FAILURE;
    return new_thread_id;
}

/**
 * Terminating all the threads in the process = delete them from any source we've saved them.
 */
void terminate_all_threads(){
    for (auto thread : all_threads){
        next_ID.push(thread.first);
        delete thread.second;
    }
    all_threads.clear();
    ready_threads.clear();
    blocked.clear();
}

/**
 * @brief Terminates the thread with ID tid and deletes it from all relevant control structures.
 *
 * All the resources allocated by the library for this thread should be released. If no thread with ID tid exists it
 * is considered an error. Terminating the main thread (tid == 0) will result in the termination of the entire
 * process using exit(0) (after releasing the assigned library memory).
 *
 * @return The function returns 0 if the thread was successfully terminated and -1 otherwise. If a thread terminates
 * itself or the main thread is terminated, the function does not return.
*/
int uthread_terminate(int tid){
    if (block_all_signals()) return FAILURE;
    // input validation
    if(tid < 0 || tid >= MAX_THREAD_NUM){
        if (unblock_all_signals()) return FAILURE;
        cerr << LIB_ERROR(INVALID_THREAD_ID);
        return FAILURE;
    }
    // check if there is such a thread in the process
    if(all_threads.count(tid) == 0){
        if (unblock_all_signals()) return FAILURE;
        cerr << LIB_ERROR(INVALID_THREAD_ID);
        return FAILURE;
    }
    // check if the given id is the main thread id, then we want to terminate all threads.
    if(tid == MAIN_THREAD_ID){
        terminate_all_threads();
        exit(0);
    }

    all_threads.erase(tid);
    for (int i = 0 ; i < ready_threads.size() ; ++i){
        // erase from ready queue
        if(ready_threads[i] == tid){
            ready_threads.erase(ready_threads.begin()+i);
        }
    }

    delete all_threads[tid];
    if(running_thread->get_id() == tid){
        running_thread = nullptr;
    }
    all_threads.erase(tid);
    switch_threads();
    sleep_threads.erase(tid); // Remove from sleep tread map
    blocked.remove(tid); // Remove from blocked list
    next_ID.push(tid);
    if (unblock_all_signals()) return FAILURE;
    return EXIT_SUCCESS;
}


/**
 * @brief Blocks the thread with ID tid. The thread may be resumed later using uthread_resume.
 *
 * If no thread with ID tid exists it is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision should be made. Blocking a thread in
 * BLOCKED state has no effect and is not considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_block(int tid){
    if (block_all_signals()) return FAILURE;
    // check if there is a Thread with ID = tid
    if(tid < 0 || tid >= MAX_THREAD_NUM){
        if (unblock_all_signals()) return FAILURE;
        cerr << LIB_ERROR(INVALID_THREAD_ID);
        return FAILURE;
    }

    // check if the thread is exists at all
    if(all_threads.count(tid) == 0){
        if (unblock_all_signals()) return FAILURE;
        cerr << LIB_ERROR(INVALID_THREAD_ID);
        return FAILURE;
    }

    // if we want to block the main thread
    if (tid == 0){
        if (unblock_all_signals()) return FAILURE;
        cerr << LIB_ERROR(BLOCK_MAIN_THREAD);
        return FAILURE;
    }

    // check if this thread is already blocked
    auto iter_blocked = std::find(blocked.begin(), blocked.end(), tid);
    if(iter_blocked != blocked.end()){
        // This tid already in blocked
        if (unblock_all_signals()) return FAILURE;
        return EXIT_SUCCESS;
    }

    // check if we want to block the current running thread
    if (running_thread->get_id() == tid){
        int ret_val = sigsetjmp(running_thread->get_env(), 1);
        if(!ret_val){
            blocked.push_front(tid);
            running_thread = nullptr;
            if (unblock_all_signals()) return FAILURE;
            time_handler(PROGRAM_SIGNAL);
        }
        return EXIT_SUCCESS;
    }

    // if thread is in ready threads queue
    if(count(ready_threads.begin(), ready_threads.end(), tid) != 0){
        remove(ready_threads.begin(), ready_threads.end(), tid);
        blocked.push_front(tid);
        if (unblock_all_signals()) return FAILURE;
        return EXIT_SUCCESS;
    }
    if (unblock_all_signals()) return FAILURE;
    return EXIT_SUCCESS;
}


/**
 * @brief Resumes a blocked thread with ID tid and moves it to the READY state.
 *
 * Resuming a thread in a RUNNING or READY state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid){
    if (block_all_signals()) return FAILURE;
    // input validation
    if(tid < 0 || tid >= MAX_THREAD_NUM){
        if (unblock_all_signals()) return FAILURE;
        cerr << LIB_ERROR(INVALID_THREAD_ID);
        return FAILURE;
    }
    // check if there is such a thread at all
    if(all_threads.count(tid) == 0){
        cerr << LIB_ERROR(INVALID_THREAD_ID);
        return FAILURE;
    }
    // check if we want to resume the current running thread or the wanted thread is in the ready threads queue
    auto is_thread_in_ready = count(ready_threads.begin(), ready_threads.end(), tid);
    if (running_thread->get_id() == tid || is_thread_in_ready != 0){
        if (unblock_all_signals()) return FAILURE;
        return EXIT_SUCCESS;
    }
    // check if the wanted thread is in the blocked list of threads
    auto iter_thread = find(blocked.begin(), blocked.end(), tid);
    if (iter_thread != blocked.end()){
        blocked.erase(iter_thread);
        // push the thread into the ready queue only if the thread is not sleeping
        if (sleep_threads.count(tid) == 0){
            ready_threads.push_back(tid);
        }
    }
    // unblock the signals and done
    if (unblock_all_signals()) return FAILURE;
    return EXIT_SUCCESS;
}


/**
 * @brief Blocks the RUNNING thread for num_quantums quantums.
 *
 * Immediately after the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY all_threads list.
 * The number of quantums refers to the number of times a new quantum starts, regardless of the reason. Specifically,
 * the quantum of the thread which has made the call to uthread_sleep isn’t counted.
 * It is considered an error if the main thread (tid==0) calls this function.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums){
    if (block_all_signals()) return FAILURE;
    // input validation
    if (num_quantums < 0){
        if (unblock_all_signals()) return FAILURE;
        cerr << SYSTEM_ERROR(INVALID_QUANTUM_NUM);
        return FAILURE;
    }
    // check if the running thread is the main thread, if so we want to exit with success
    if (running_thread->get_id() == 0){
        if (unblock_all_signals()) return FAILURE;
        cerr << LIB_ERROR(SLEEP_MAIN_THREAD);
        return FAILURE;
    }
    // else, we will update the place of the running thread and switch
    sleep_threads[running_thread->get_id()] = num_quantums;
    int ret_val = sigsetjmp(running_thread->get_env(), 1);
    if(ret_val == 0){
        running_thread = nullptr;
        time_handler(PROGRAM_SIGNAL);
    }
    if (unblock_all_signals()) return FAILURE;
    return EXIT_SUCCESS;
}


/**
 * This function returns the thread ID of the calling thread.
*/
int uthread_get_tid(){
    return running_thread->get_id();
}


/**
 * @brief Returns the total number of quantums since the library was initialized, including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should be increased by 1.
 *
 * @return The total number of quantums.
*/
int uthread_get_total_quantums(){
    return process_quantum_counter;
}


/**
 * @brief Returns the number of quantums the thread with ID tid was in RUNNING state.
 *
 * On the first time a thread runs, the function should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state when this function is called, include
 * also the current quantum). If no thread with ID tid exists it is considered an error.
 *
 * @return On success, return the number of quantums of the thread with ID tid. On failure, return -1.
*/
int uthread_get_quantums(int tid){
    if (block_all_signals()) return FAILURE;
    // input validation
    if (tid < 0 || tid >= MAX_THREAD_NUM) {
        cerr << LIB_ERROR(INVALID_THREAD_ID);
        return FAILURE;
    }
    // check if there is exists such a thread
    if (all_threads.find(tid) == all_threads.end()) {
        if (unblock_all_signals()) return FAILURE;
        cerr << LIB_ERROR(INVALID_THREAD_ID);
        return FAILURE;
    }
    int running_counter = all_threads[tid]->get_running_mode_counter();
    if (unblock_all_signals()) return FAILURE;
    return running_counter;
}

