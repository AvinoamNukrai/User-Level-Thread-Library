/*
 * Interval-timer demo program.
 * Hebrew University OS course.
 * Author: OS, os@cs.huji.ac.il
 */

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>


int gotit = 0;


void timer_handler(int sig)
{
    gotit = 1;
    printf("Timer expired\n");
}


int main(void)
{
    struct sigaction sa = {0};
    struct itimerval timer;

    // Install timer_handler as the signal handler for SIGVTALRM.
    sa.sa_handler = &timer_handler;
    if (sigaction(SIGVTALRM, &sa, NULL) < 0)
    {
        printf("sigaction error.");
    }

    // Configure the timer to expire after 1 sec... */
    timer.it_value.tv_sec = 1;        // first time interval, seconds part
    timer.it_value.tv_usec = 0;        // first time interval, microseconds part

    // configure the timer to expire every 3 sec after that.
    timer.it_interval.tv_sec = 3;    // following time intervals, seconds part
    timer.it_interval.tv_usec = 0;    // following time intervals, microseconds part

    // Start a virtual timer. It counts down whenever this process is executing.
    if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
    {
        printf("setitimer error.");
    }

    for (;;)
    {
        if (gotit)
        {
            printf("Got it!\n");
            gotit = 0;
        }
    }
}
