#include "thread_pool.h"


thread_pool::thread_pool(int num) : size(num), is_shutdown(0) {
    if (num <= 0) {
        fprintf(stderr, "the num of working threads is incorrect\n");
        exit(1);
    }
    threads.resize(num);
    for (int i = 0; i < num; i++) {
        pthread_create(&threads[i], NULL, work, this);
        //pthread_detach(threads[i]);
    }    
}

thread_pool::thread_pool() : thread_pool(default_size) {}

thread_pool::~thread_pool() {
    is_shutdown = 1;
    for (int i = 0; i < size; i++)
        sem.post();
    for (int i = 0; i < size; i++)
        pthread_join(threads[i], NULL);
}

void* thread_pool::work(void* arg) {
    thread_pool* pool = (thread_pool*)arg;
    pool->run();
    return pool;
}

void thread_pool::run() {
    while (true) {
        sem.wait();
        if (is_shutdown) {
            break;
        }

        lock.lock();
        if (tasks.empty()) {
            lock.unlock();
            continue;
        }
        task tmp = tasks.front();
        tasks.pop_front();
        lock.unlock();
        tmp.fun(tmp.arg);
    }
}

void thread_pool::add_job(void (*fun)(void*), void* arg) {
    lock.lock();
    task tmp;
    tmp.fun = fun;
    tmp.arg = arg;
    tasks.push_back(tmp);
    lock.unlock();
    sem.post();
}