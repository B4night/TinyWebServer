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
    };
    struct client_info clients[MAX_FD_NUM];
    int max_idx;
private:
    int epfd;
    int lfd;
private:
    struct epoll_event events[MAX_FD_NUM];
public:
    epoll_frame(int num, int port = 4000);
    void dispatch();
    void set_timer(int interval);
    static void drop_inactive(void*);
public:    //SIGALRM handler
    static int pipefd[2];
    static void alarm_handler(int);
private:
    struct transfer_arg {
        epoll_frame* ef;
        int fd;
    };
private:
    thread_pool tp;
private:
    void init_clients();
    void init_socket(int p);
    static void accept_client(void* arg);
    void __accept_client();
    static void communicate(void* arg);
    void __comminicate(int fd);
};

#endif