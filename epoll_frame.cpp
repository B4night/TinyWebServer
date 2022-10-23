#include "epoll_frame.h"

void sys_err(const char* str) {
    perror(str);
    exit(1);
}

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFD);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFD, flags);
}

epoll_frame::epoll_frame(int num, int port) {
    printf("server running on the port:%d\n", port);
    end = 0;
    epfd = epoll_create(num);
    if (epfd == -1) {
        sys_err("epoll_create error\n");
    }
    init_listen_socket(port);
}
void epoll_frame::init_listen_socket(int port) {
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
        sys_err("socket error\n");
    set_nonblock(lfd);

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(lfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        sys_err("bind error\n");
    if (listen(lfd, MAX_FD_NUM) == -1)
        sys_err("listen error\n");
    eventset(&my_evs[MAX_FD_NUM], lfd, acceptconnect, &my_evs[MAX_FD_NUM]);
    eventadd(EPOLLIN, &my_evs[MAX_FD_NUM]);
}
void epoll_frame::eventset(my_event* me, int fd, void (*func)(void*), void* arg) {
    me->m_fd = fd;
    me->my_event = 0;
    me->cb = func;
    me->ef = this;
    me->m_arg = arg;
    me->status = 0;
    me->m_lasttime = time(NULL);
}
void epoll_frame::eventadd(int event, my_event* me) {
    struct epoll_event ev;
    ev.data.ptr = me;
    ev.events = event;
    me->my_event = event;
    int OP;

    if (me->status == 0) {
        OP = EPOLL_CTL_ADD;            
    } else {
        fprintf(stderr, "already on epoll tree\n");
        exit(1);
    }

    if (epoll_ctl(epfd, OP, me->m_fd, &ev) == -1)
        sys_err("epoll_ctl error\n");
    else {
        printf("event add success: fd=[%d]\n", me->m_fd);
        me->status = 1;
    }
}
void epoll_frame::acceptconnect(void* arg) {
    my_event* me = (my_event*)arg;
    epoll_frame* ef = me->ef;
    ef->__acceptconnect(me->m_fd, me->my_event);
}
void epoll_frame::__acceptconnect(int fd, int event) {
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int connfd = accept(lfd, (struct sockaddr*)&client_addr, &len);
    if (connfd == -1)
        sys_err("accept error\n");
    do {
        int i;
        for (i = 0; i < MAX_FD_NUM; i++)
            if (my_evs[i].status == 0)
                break;
        if (i == MAX_FD_NUM) {
            fprintf(stderr, "my_evs is full\n");
            exit(1);
        }
        
        set_nonblock(connfd);
        eventset(&my_evs[i], connfd, recvdata, &my_evs[i]);
        eventadd(EPOLLIN, &my_evs[i]);
    } while (0);
    char ip[16];
    fprintf(stdout, "client addr: %s:%d\n", inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip, 16), ntohs(client_addr.sin_port));
    return;

}

void epoll_frame::eventdel(my_event* me) {
    if (me->status != 1)
        return;
    epoll_ctl(epfd, EPOLL_CTL_DEL, me->m_fd, NULL);
    me->status = 0;
    return;
}

void epoll_frame::recvdata(void* arg) {
    my_event* me = (my_event*)arg;
    epoll_frame* ef = me->ef;
    ef->__recvdata(me->m_fd, me->my_event, me);
}    
void epoll_frame::__recvdata(int fd, int event, my_event* me) {
    int n = read(me->m_fd, me->m_buf, me->BUF_SIZE);
    if (n > 0) {
        me->m_buf_len = n;
        me->m_buf[n] = 0;

        eventdel(me);
        eventset(me, me->m_fd, senddata, me);
        eventadd(EPOLLOUT, me);
    } else if (n == 0) {
        eventdel(me);
        close(me->m_fd);
    }
}

void epoll_frame::senddata(void* arg) {
    my_event* me = (my_event*)arg;
    epoll_frame* ef = me->ef;
    ef->__senddata(me->m_fd, me->my_event, me);
}
void epoll_frame::__senddata(int fd, int event, my_event* me) {
    /*
    handle me->m_buf
    */
    int n = write(me->m_fd, me->m_buf, me->m_buf_len);
    if (n > 0) {
        eventdel(me);
        eventset(me, me->m_fd, recvdata, me);
        eventadd(EPOLLIN, me);
    } else {
        eventdel(me);
        close(me->m_fd);
    }
}

void epoll_frame::dispatch() {
    while (1) {
        if (end)
            break;
        int num = epoll_wait(epfd, evs, MAX_FD_NUM, -1);
        if (num <= 0) {
            sys_err("epoll_wait error \n");
        }
        for (int i = 0; i < num; i++) {
            my_event* tmp = (my_event*)evs[i].data.ptr;
            //可以用线程池改写
            tmp->cb(tmp->m_arg);
        }
    }
}

void epoll_frame::quit() {
    end = 1;
}