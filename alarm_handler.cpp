#include "alarm_handler.h"

#include <stdio.h>

extern void sys_err(const char*);

void init(void (*cb)(int)) {
    struct sigaction act;
    act.sa_handler = cb;
    sigaction(SIGALRM, &act, NULL);
}  

void set_interval(int interval) {
    struct itimerval t;
    t.it_value.tv_sec = interval;
    t.it_value.tv_usec = 0;
    t.it_interval.tv_sec = interval;
    t.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &t, NULL);
}