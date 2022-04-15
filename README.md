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
