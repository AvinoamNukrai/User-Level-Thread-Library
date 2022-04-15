# User-Level-Thread-Library
A user-level thread library, implements the Round-Robin algorithm. 

Introduction:
  I have implemented a static library, that creates and manages user-level threads.
  A potential user will be able to include my library and use it according to the public interface: the uthreads.h header file. 

The Threads in the library:
  Initially, a program is comprised of the default main thread, whose ID is 0. All other threads will be explicitly created. 
  Each existing thread has a unique thread ID, which is a non-negative integer. The ID given to a new thread must be the
  smallest non-negative integer not already taken by an existing thread (i.e. if thread with ID 1 is terminated and then a new thread
  is spawned, it should receive 1 as its ID). The maximal number of threads the library should 
  support (including the main thread) is MAX_THREAD_NUM.

Thread State Diagram:
  At any given time during the running of the user's program, each thread in the program is in one of the 
  states shown in the following state diagram - RUNNING, READY, BLOCKED and SLEEP. Transitions from state to 
  state occur as a result of calling one of the library functions, or from elapsing of time. 
  ![threads_diagram_state](https://user-images.githubusercontent.com/64755588/163553980-d924e197-e17f-43f6-b2e3-5303c05c25b7.png)


Scheduler (Signals Handler):
  In order to manage the threads in the library, I defined a Scheduler. The scheduler will implement the Round-Robin (RR)
  scheduling algorithm. States Each thread can be in one of the following states: RUNNING, BLOCKED, SLEEP or READY. Time
  The process running time is measured by the Virtual Timer.

Roound-Robin Algorithm: 
  The Round-Robin scheduling policy should work as follows:
    • Every time a thread is moved to RUNNING state, it is allocated a predefined number of micro-seconds to run.
      This time interval is called a quantum.
    • The RUNNING thread is preempted if any of the following occurs:
        a) Its quantum expires.
        b) It changed its state to BLOCKED and is consequently waiting for an event (i.e. some other thread that will resume
        it or after some time has passed – more details below).
        c) It is terminated.
    • When the RUNNING thread is preempted, do the following:
        1. If it was preempted because its quantum has expired, move it to the end of the READY threads list.
        2. Move the next thread in the queue of READY threads to RUNNING state.
    • Every time a thread moves to the READY state from any other state, it is placed at the end of the list of READY threads.
    • When a thread doesn't finish its quantum (as in the case of a thread that blocks itself), 
      the next thread should start executing immediately as if the previous thread finished its quota.
      In the following illustration the quantum was set for 2 seconds, Thread 1 blocks itself
      after running only for 1 second and Thread 2 immediately starts its next quantum.

    ![threads_diagram](https://user-images.githubusercontent.com/64755588/163555242-6ae77bae-2164-4267-b465-c7a7caa688e3.png)

The code was tested well, 
Enjoy!

   
