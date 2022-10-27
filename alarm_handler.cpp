#include "alarm_handler.h"

#include <stdio.h>

extern void sys_err(const char*);

void init(void (*cb)(int)) {    // 设置SIGALRM的信号处理函数
    struct sigaction act;
    act.sa_handler = cb;
    sigaction(SIGALRM, &act, NULL);
}  

void set_interval(int interval) {   // 每隔interval秒程序会收到一个SIGALRM信号
    struct itimerval t; 
    t.it_value.tv_sec = interval;
    t.it_value.tv_usec = 0;
    t.it_interval.tv_sec = interval;
    t.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &t, NULL);
}