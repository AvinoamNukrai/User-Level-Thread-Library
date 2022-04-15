// H file for the Thread class

#ifndef EX2_3_THREAD_H
#define EX2_3_THREAD_H

// Imports & Typedefs
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
typedef unsigned long address_t;


// Thread class
class Thread{

    public:
    int id_; // The ID of the thread
    int running_mode_counter_; // counter of how many quantum the thread was in running mode
    int thread_quantum_counter_; // life counter of the thread (in terms of quantum)
    char* stack_; // The stack of the thread
    sigjmp_buf env_{}; // Place for saving all the context of the thread

    public:

    // ctor
    Thread(int id, thread_entry_point entry_point);

    // getters and setters
    int get_running_mode_counter();

    int get_id();

    sigjmp_buf& get_env();

    void inc_running_mode_counter();

    void inc_thread_quantum_counter();

    // Dtor
    ~Thread();

};


#endif //EX2_3_THREAD_H
