// Imports
#include "Thread.h"
#define JB_SP 6
#define JB_PC 7

/*
 * Code that translate an address of functions in memory into
 * a real number (was taken from outside source file), both for 64 bit and 32 bit.
 */

#ifdef __x86_64__
/* code for 64 bit Intel arch */

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif


/**
 * Ctor for the Thread class
 * @param id  - the id of the new thread
 * @param entry_point - the entry point of the new thread
 */
Thread::Thread(int id, thread_entry_point entry_point): id_(id), running_mode_counter_(0), thread_quantum_counter_(1){
    // Allocate the stack on the heap
    stack_ = new char[STACK_SIZE];
    // Creates address of the pointers of the Thread
    address_t sp = (address_t) stack_ + STACK_SIZE - sizeof(address_t);
    address_t pc = (address_t) entry_point;
    // Saving the context of the new Thread
    int ret_val = sigsetjmp(env_, 1);
    if (ret_val == 0){
        // Load the pointers into env_ (the place in which we save the all context)
        (env_->__jmpbuf)[JB_SP] = translate_address(sp);
        (env_->__jmpbuf)[JB_PC] = translate_address(pc);
        sigemptyset(&env_->__saved_mask);
    }
}

/**
 * returns the running_mode_counter_ of the thread
 */
int Thread::get_running_mode_counter(){
    return running_mode_counter_;
}

/**
 * returns the id of the thread
 */
int Thread::get_id(){
    return id_;
}

/**
 * returns the env_ of the thread
 */
sigjmp_buf& Thread::get_env(){
    return this->env_;
}

/**
 * Increment the running_mode_counter_ of the thread
 */
void Thread::inc_running_mode_counter(){
    running_mode_counter_++;
}

/**
 * Increment the thread_quantum_counter_ of the thread
 */
void Thread::inc_thread_quantum_counter(){
    ++thread_quantum_counter_;
}

/**
 * Dtor
 */
Thread::~Thread() {
    delete[] stack_;
    stack_ = nullptr;
}

