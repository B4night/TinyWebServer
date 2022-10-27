#ifndef __EPOLL_FRAME_H_
#define __EPOLL_FRAME_H_

#include <sys/epoll.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "thread_pool.h"
#define MAX_FD_NUM 1024

class epoll_frame {
private:
    struct client_info {    
        struct sockaddr_in addr;
        char ip[16];
        int port;
        int fd;
        time_t last_active; // 最后活动的事件
    };
    struct client_info clients[MAX_FD_NUM];
    int max_idx;    // max_idx的值为clients数组中有效坐标的最大值
private:
    int epfd;
    int lfd;    // listening fd
    struct epoll_event events[MAX_FD_NUM];  // epoll_wait使用

public:
    epoll_frame(int num, int port = 4000);  // 构造函数
    void dispatch();                        
    void set_timer(int interval);           // 设置定时器
    void drop_inactive();                   // 有SIGALRM信号时调用
public:    //SIGALRM handler
    static int pipefd[2];                   // 定时器使用管道来通知信号到来
    static void alarm_handler(int);         // 信号处理函数
private:
    int global_interval;    // SIGALRM信号到来的间隔
private:
    thread_pool tp;         // 线程池
private:
    void init_clients();    // 初始clients数组
    void init_socket(int p);// socket, bind, listen

    static void accept_client(void* arg);   // lfd上有EPOLLIN事件时的调用的函数
    void __accept_client();                 // 被accept_client调用,实际处理逻辑的函数
private:
    struct transfer_arg {   // communicate函数的参数
        epoll_frame* ef;
        int fd;
    };
    static void communicate(void* arg); // client fd上有EPOLLIN事件时的调用的函数
    void __comminicate(int fd);         // 被communicate调用, 实际处理逻辑的函数
};

#endif