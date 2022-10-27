#include "epoll_frame.h"

extern void init(void (*)(int));
extern void set_interval(int);
extern void set_cb(void (*cb)(void*), void*);
extern void solve_request(int fd, char* buf, int len);
extern void inactive_handler(int fd);

void sys_err(const char* str) {
    perror(str);
    exit(1);
}

void setnonblock(int fd) {
    int flag = fcntl(fd, F_GETFD);
    flag |= O_NONBLOCK;
    fcntl(fd, F_SETFD, flag);
}

int epoll_frame::pipefd[2] = {-1, -1};

void epoll_frame::alarm_handler(int sig) {
    int saved = errno;
    write(pipefd[1], &sig, 1);
    errno = saved;
}

void epoll_frame::set_timer(int interval) {
    pipe(pipefd);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = pipefd[0];
    if ((epoll_ctl(epfd, EPOLL_CTL_ADD, pipefd[0], &ev) == -1))
        sys_err("epoll_ctl error");

    init(epoll_frame::alarm_handler);
    set_interval(interval);
    global_interval = interval;
}

void epoll_frame::drop_inactive() {
    for (int i = 0; i <= max_idx; i++) {
        if (time(NULL) - clients[i].last_active > global_interval) {
            inactive_handler(clients[i].fd);
            epoll_ctl(epfd, EPOLL_CTL_DEL, clients[i].fd, NULL);
            clients[i].fd = -1;
            if (max_idx == i)
                while (clients[max_idx].fd == -1)
                    max_idx--;
        }
    }
}

epoll_frame::epoll_frame(int num, int port) : tp(2) {
    epfd = epoll_create(num);
    if (epfd == -1)
        sys_err("epoll_create fail");
    init_socket(port);
    init_clients();
}

void epoll_frame::dispatch() {
    while (true) {
        int num = epoll_wait(epfd, events, MAX_FD_NUM, -1);
        if (num <= 0) {
            if (errno == EINTR)     //block interrupted system call
                continue;
            sys_err("epoll_wait error");
        }
        bool alarm = false;
        for (int i = 0; i < num; i++) {
            if (events[i].data.fd == lfd) {
                // accept_client(this);
                tp.add_job(accept_client, this);
            } else if (events[i].data.fd == pipefd[0]) {
                char buf[16];
                read(pipefd[0], buf, 16);   //read the data 
                alarm = true;
            } else {
                transfer_arg ta;
                ta.ef = this;
                ta.fd = events[i].data.fd;
                // communicate(&ta); 
                tp.add_job(communicate, &ta);
            }
            usleep(15);
        }
        if (alarm)
            drop_inactive();    //do SIGALRM call back function
    }
}

void epoll_frame::init_clients() {
    for (int i = 0; i < MAX_FD_NUM; i++)
        clients[i].fd = -1;
}

void epoll_frame::init_socket(int p) {
    printf("server running on port %d\n", p);

    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
        sys_err("socket error");
    // setnonblock(lfd);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(p);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(lfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        sys_err("bind error");
    if (listen(lfd, MAX_FD_NUM) == -1)
        sys_err("listen error");
    
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = lfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev) == -1)
        sys_err("epoll_ctl error");
}

void epoll_frame::accept_client(void* arg) {
    epoll_frame* ef = (epoll_frame*)arg;
    ef->__accept_client();
}

void epoll_frame::__accept_client() {
    int i;
    for (i = 0; i < MAX_FD_NUM; i++)
        if (clients[i].fd == -1)
            break;
    clients[i].last_active = time(NULL);
    if (i > max_idx)
        max_idx = i;
    if (i == MAX_FD_NUM) {
        fprintf(stderr, "clients are full\n");
        exit(1);
    }
    socklen_t len = sizeof(clients[i].addr);
    clients[i].fd = accept(lfd, (struct sockaddr*)&clients[i].addr, &len);

    inet_ntop(AF_INET, &clients[i].addr.sin_addr.s_addr, clients[i].ip, 16);
    clients[i].port = ntohs(clients[i].port);

    printf("Clint addr: %s:%d\n", clients[i].ip, clients[i].port);

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = clients[i].fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, clients[i].fd, &ev) == -1)
        sys_err("epoll_ctl add client error");
}

void epoll_frame::communicate(void* arg) {
    transfer_arg* ta = (transfer_arg*)arg;
    ta->ef->__comminicate(ta->fd);
}

void epoll_frame::__comminicate(int fd) {
    int i;
    for (i = 0; i <= max_idx; i++)
        if (clients[i].fd == fd)
            break;
    clients[i].last_active = time(NULL);
    char buf[2048];
    int n = read(fd, buf, 2048);

    printf("\n\n");

    for (int k = 0; k < n; k++) {
        printf("%c", buf[k]);
    }

    printf("\n\n");

    if (n <= 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            return;

        printf("Disconnect with %s:%d\n", clients[i].ip, clients[i].port);
        clients[i].fd = -1;
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        if (i == max_idx)
            while (clients[max_idx].fd == -1)
                max_idx--;
        return;
    }

    //this part for http parser
    // write(fd, buf, n);
    solve_request(fd, buf, n);
}