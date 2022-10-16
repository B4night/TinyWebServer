#ifndef _PTHREAD_POLL_H_
#define _PTHREAD_POLL_H_

#include <list>
#include <vector>
#include <unistd.h>
#include "locker.h"

class thread_pool {
private:
    struct task {
        void (*fun)(void*);
        void* arg;
    };
private:
    int size;       //the num of working threads
    MyMutex lock;       //a mutex
    MySemaphore sem;    //semaphore to indicate the num of jobs
    std::list<task> tasks;
    std::vector<pthread_t> threads; 

    const int default_size = 16;
    int is_shutdown;
public:
    thread_pool();
    thread_pool(int num);
    ~thread_pool();
    void add_job(void (*fun)(void*), void* arg);
private:
    static void* work(void* arg);   //the working threads' call back
    void run();                     //the actual function that work() calls
};

#endif