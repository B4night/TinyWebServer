#include "epoll_frame.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <ctype.h>


int epfd;
void sys_err(const char*);
struct clientInfo clients[NUM];
struct my_events m_events[NUM + 1];
int lfd;
int port;
int maxIdx;
extern void solve_GET(int, char*, int);

void Epoll_create() {
    epfd = epoll_create(NUM);
    if (epfd == -1)
        sys_err("epoll_create error");
}

void init_clients() {
    for (int i = 0; i < NUM; i++) {
        clients[i].connfd = -1;
        m_events[i].m_fd = -1;
    }
}

void set_socket() {
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
        sys_err("socket error");
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;    //make the port reusable
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(lfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        sys_err("bind error");
    if (listen(lfd, NUM) == -1)
        sys_err("listen error");

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = &m_events[NUM];

    
    init_clients();
    m_event_set(&m_events[NUM], lfd, &m_events[NUM], lfd_cb);    
    epfd_add(EPOLLIN, &m_events[NUM]);
}

void epfd_add(int event, struct my_events* m_ev) {
    m_ev->m_event = event;
    struct epoll_event ev;
    ev.events = event;
    ev.data.ptr = m_ev;

    if (m_ev->m_status == 1) {
        fprintf(stderr, "epfd_add error: already in epfd rb-tree\n");
        exit(1);
    }
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, m_ev->m_fd, &ev) == -1) {
        sys_err("epoll_ctl error");
    } else {
        fprintf(stdout, "epfd add success:lfd = [%d]\n", m_ev->m_fd);
        m_ev->m_status = 1;
    }
    
}

void epfd_del(struct my_events* m_ev) {
    if (m_ev->m_status == 0)
        return;
    
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, m_ev->m_fd, NULL) == -1) {
        sys_err("epoll_ctl delete error");
    } else {
        m_ev->m_status = 0;
    }
}

void lfd_cb(int fd, int event, void* arg) {
    int i;
    for (i = 0; i < NUM; i++)
        if (clients[i].connfd == -1)
            break;
    if (i > maxIdx)
        maxIdx = i;
    if (i == NUM) {
        fprintf(stderr, "clients are full\n");
        exit(1);
    }
    
    struct clientInfo* cli = &clients[i];
    int len = sizeof(cli->clientAddr);
    cli->connfd = accept(fd, (struct sockaddr*)&cli->clientAddr, &len);
    if (cli->connfd == -1) {
        sys_err("accept error");
    }
    
    cli->port = ntohs(cli->clientAddr.sin_port);
    inet_ntop(AF_INET, &cli->clientAddr.sin_addr.s_addr, cli->ip, 16);
    
    m_event_set(&m_events[i], cli->connfd, &m_events[i], client_recv_cb);
    epfd_add(EPOLLIN, &m_events[i]);

    fprintf(stdout, "connect with %s:%d\n", cli->ip, cli->port);
}

void client_recv_cb(int fd, int event, void* arg) {
    struct my_events* m_ev = (struct my_events*)arg;
    int n = read(fd, m_ev->m_buf, sizeof(m_ev->m_buf));
    if (n <= 0) {
        epfd_del(m_ev);
        close(m_ev->m_fd);
    } else if (n > 0) {
        m_ev->m_buf_len = n;
        m_ev->m_buf[n] = 0;
        epfd_del(m_ev);
        m_event_set(m_ev, m_ev->m_fd, m_ev, client_send_cb);
        epfd_add(EPOLLOUT, m_ev);
    }
}

void client_send_cb(int fd, int event, void* arg) {
    struct my_events* m_ev = (struct my_events*)arg;

    //handle data
    solve_GET(m_ev->m_fd, m_ev->m_buf, m_ev->m_buf_len);

    // int n = write(m_ev->m_fd, m_ev->m_buf, m_ev->m_buf_len);

    epfd_del(m_ev);
    m_event_set(m_ev, m_ev->m_fd, m_ev, client_recv_cb);
    epfd_add(EPOLLIN, m_ev);


}

void Epoll_loop() {
    struct epoll_event events[NUM];
    while (1) {
        int num = epoll_wait(epfd, events, NUM, -1);
        if (num <= 0) {
            sys_err("epoll_wait error");
        }
        for (int i = 0; i < num; i++) {
            struct my_events* m_ev = events[i].data.ptr;
            m_ev->m_cb(m_ev->m_fd, events[i].events, m_ev);
        }
    }
}