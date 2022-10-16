#ifndef _LOCKER_H_
#define _LOCKER_H_

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>

class MyMutex {
private:
    pthread_mutex_t mutex;
public:
    MyMutex() {
        int ret;
        if ((ret = pthread_mutex_init(&mutex, NULL)) != 0) {
            fprintf(stderr, "pthread_mutex_init error:%s\n", strerror(ret));
            exit(1);
        }
    }
    ~MyMutex() {
        int ret;
        if ((ret =  pthread_mutex_destroy(&mutex)) != 0) {
            fprintf(stderr, "pthread_mutex_destroy error:%s\n", strerror(ret));
            exit(1);
        }
    }
    void lock() {
        int ret;
        if ((ret =  pthread_mutex_lock(&mutex)) != 0) {
            fprintf(stderr, "pthread_mutex_lock error:%s\n", strerror(ret));
            exit(1);
        }
    }
    void unlock() {
        int ret;
        if ((ret =  pthread_mutex_unlock(&mutex)) != 0) {
            fprintf(stderr, "pthread_mutex_unlock error:%s\n", strerror(ret));
            exit(1);
        }
    }
};

class MySemaphore {
private:
    sem_t sem;
public:
    MySemaphore() {
        int ret;
        if ((ret = sem_init(&sem, 0, 0)) != 0) {
            fprintf(stderr, "sem_init error:%s\n", strerror(ret));
            exit(1);
        }
    }
    MySemaphore(int num) {
        int ret;
        if ((ret = sem_init(&sem, 0, num)) != 0) {
            fprintf(stderr, "sem_init error:%s\n", strerror(ret));
            exit(1);
        }
    }
    ~MySemaphore() {
        int ret;
        if ((ret = sem_destroy(&sem)) != 0) {
            fprintf(stderr, "sem_destroy error:%s\n", strerror(ret));
            exit(1);
        }
    }
    void wait() {
        int ret;
        if ((ret = sem_wait(&sem)) != 0) {
            fprintf(stderr, "sem_wait error:%s\n", strerror(ret));
            exit(1);
        }
    }
    void post() {
        int ret;
        if ((ret = sem_post(&sem)) != 0) {
            fprintf(stderr, "sem_post error:%s\n", strerror(ret));
            exit(1);
        }
    }
};

class MyCond {
private:
    pthread_cond_t cond;
public:
    MyCond() {
        int ret;
        if ((ret = pthread_cond_init(&cond, NULL)) != 0) {
            fprintf(stderr, "pthread_cond_init error:%s\n", strerror(ret));
            exit(1);
        }
    }
    ~MyCond() {
        int ret;
        if ((ret = pthread_cond_destroy(&cond)) != 0) {
            fprintf(stderr, "pthread_cond_destroy error:%s\n", strerror(ret));
            exit(1);
        }
    }
    void wait(pthread_mutex_t* mutex) {
        int ret;
        if ((ret = pthread_cond_wait(&cond, mutex)) != 0) {
            fprintf(stderr, "pthread_cond_wait error:%s\n", strerror(ret));
            exit(1);
        }
    }
    void signal() {
        int ret;
        if ((ret = pthread_cond_signal(&cond)) != 0) {
            fprintf(stderr, "pthread_cond_signal error:%s\n", strerror(ret));
            exit(1);
        }
    }
};

#endif 