#ifndef __EPOLL_FRAME_H_
#define __EPOLL_FRAME_H_

#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

#define MAX_FD_NUM 1024

class epoll_frame {
private:
    class my_event {
    public:
        int my_event;
        int m_fd;
        void (*cb)(void* arg);    //第一个参数为this
        void* m_arg;

        epoll_frame* ef;

        static const int BUF_SIZE = 4096;
        char m_buf[BUF_SIZE];
        int m_buf_len;
        int status;

        time_t m_lasttime;
    };

private:
    int epfd;
    int lfd;   
    my_event my_evs[MAX_FD_NUM + 1]; 
    epoll_event evs[MAX_FD_NUM];

    bool end;
public:
    epoll_frame(int num = 128, int port = 4000);
    void dispatch();
    void quit();
private:
    void init_listen_socket(int port);
    void eventset(my_event* me, int fd, void (*func)(void*), void* arg);
    void eventadd(int event, my_event* me);
    static void acceptconnect(void* arg);
    void __acceptconnect(int fd, int event);
    void eventdel(my_event* me);
    static void recvdata(void* arg);
    void __recvdata(int fd, int event, my_event* me);
    static void senddata(void* arg);
    void __senddata(int fd, int event, my_event* me);
};

#endif