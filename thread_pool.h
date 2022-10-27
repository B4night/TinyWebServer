#ifndef _THREAD_POLL_H_
#define _THREAD_POLL_H_

#include <list>
#include <vector>
#include <unistd.h>
// #include "locker.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>

class thread_pool {
private:
    struct task {   // 人物列表
        void (*fun)(void*);
        void* arg;
    };
private:
    int size;       //the num of working threads
    pthread_mutex_t lock;       //a mutex
    // sem_t sem;    //semaphore to indicate the num of jobs
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    std::list<task> tasks;
    std::vector<pthread_t> threads; 

    const int default_size = 16;
    int is_shutdown;
public:
    thread_pool();
    thread_pool(int num);
    ~thread_pool();
    void add_job(void (*fun)(void*), void* arg);    // 往线程池中添加任务, 回调函数fun, 参数为arg
private:
    static void* work(void* arg);   //the working threads' call back
    void run();                     //the actual function that work() calls
};

#endif