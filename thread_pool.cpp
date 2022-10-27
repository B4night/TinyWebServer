#include "thread_pool.h"


thread_pool::thread_pool(int num) : size(num), is_shutdown(0) {
    if (num <= 0) {
        fprintf(stderr, "the num of working threads is incorrect\n");
        exit(1);
    }
    pthread_mutex_init(&lock, NULL);
    // sem_init(&sem, 0, 0);
    pthread_cond_init(&not_empty, NULL);
    pthread_cond_init(&not_full, NULL);

    threads.resize(num);
    pthread_mutex_lock(&lock);
    for (int i = 0; i < num; i++) {
        pthread_create(&threads[i], NULL, work, this);  // 创建线程
        // printf("thread %d is created\n", threads[i]);
        pthread_detach(threads[i]);        // detach线程
    }    
    pthread_mutex_unlock(&lock);
}

thread_pool::thread_pool() : thread_pool(default_size) {}

thread_pool::~thread_pool() {
    is_shutdown = 1;
    for (int i = 0; i < size; i++) {
        // sem_post(&sem);
        pthread_cond_signal(&not_empty);
    }
    pthread_mutex_destroy(&lock);
    // sem_destroy(&sem);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);
}

void* thread_pool::work(void* arg) {    // 工作线程调用
    thread_pool* pool = (thread_pool*)arg;
    pool->run();
    return pool;
}

void thread_pool::run() {   // 工作线程处理逻辑的函数
    while (true) {
        printf("in func %s\n", __func__);
        pthread_mutex_lock(&lock);
        while (tasks.empty()) {
            pthread_cond_wait(&not_empty, &lock);
            if (this->is_shutdown)
                return;
        }
        task tmp = tasks.front();   // 从任务列表中取出任务, 执行
        tasks.pop_front();
        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&lock);
        tmp.fun(tmp.arg);
    }
}

void thread_pool::add_job(void (*fun)(void*), void* arg) {  // 向线程池中添加任务
    printf("in func %s\n", __func__);
    pthread_mutex_lock(&lock);
    task tmp;
    tmp.fun = fun;
    tmp.arg = arg;
    tasks.push_back(tmp);
    // sem_post(&sem);
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&lock);
}