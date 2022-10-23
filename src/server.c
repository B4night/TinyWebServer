#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#define NUM 1024

struct clientInfo { //information about clients
    struct sockaddr_in clientAddr;
    int connfd;
    char ip[16];
    int port;
    char buf[4096];
};
const int bufSize = 4096;

struct clientInfo clients[NUM];
int maxIdx = -1;

int epfd;   //return value of rpoll_create
int port;   //server port
char path[256]; //the working directory
int lfd;    //listen fd

void sys_err(const char* str) {
    perror(str);
    exit(1);
}

void Epoll_create();    //create a epfd
void InitClients();     //initiate clients
void Set_socket();      //initiate a socket
void do_Accept();       //accept clint connection
void Disconnect(int idx);   //disconnect with clients
void do_Commu(int connfd);  //communicate with clients specified by connfd
void Epoll_loop();          //loop with epoll_wait
void init_POST(int connfd); //the information POSTed to the clients after connection
void POST(int, const char*);    //POST information
void solve_GET(int connfd, char* buf, int size);    //solve GET

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <port> <source file path>\n", argv[0]);
        exit(1);
    }
    
    port = atoi(argv[1]);
    if (chdir(argv[2]) == -1)
        sys_err("chdir error");
    getcwd(path, 256);
    //fprintf(stdout, "pwd:%s\n", path);

    Epoll_create();
    Set_socket();
    Epoll_loop();

    return 0;
}

void Epoll_loop() {
    int startTime = time(NULL);
    struct epoll_event events[NUM];
    struct epoll_event ev;
    while (1) {
        if (time(NULL) - startTime > 100) {
            fprintf(stderr, "no events in the past 100 seconds, server shut down...\n");
            fflush(stdout);
            break;
        }
        int num = epoll_wait(epfd, events, NUM, 1000);
        if (num < 0)
            perror("epoll_wait error");
        if (num == 0)
            continue;
        startTime = time(NULL);

        for (int i = 0; i < num; i++) {
            if (events[i].data.fd == lfd) {
                do_Accept();
            } else {
                do_Commu(events[i].data.fd);
            }
        }

    }
}

void do_Commu(int connfd) {
    int i;
    for (i = 0; i <= maxIdx; i++) {
        if (clients[i].connfd == connfd)
            break;
    }
    char* buf = clients[i].buf;
    int n = read(connfd, buf, bufSize);
    //printf("%s\n", buf);
    if (n == 0) {
        Disconnect(i);
    } else if (n > 0) {
        buf[n] = 0;
        solve_GET(connfd, buf, 4096);
    }

}

void Disconnect(int idx) {
    fprintf(stdout, "disconnect with %s:%d\n", clients[idx].ip, clients[idx].port);
    int connfd = clients[idx].connfd;
    clients[idx].connfd = -1;
    epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
    close(connfd);
    if (idx == maxIdx)
        while (maxIdx != -1 && clients[--maxIdx].connfd != -1)
            ;
}

void do_Accept() {
    int i = 0;
    for (; i < NUM; i++) {
        if (clients[i].connfd == -1)
            break;
    }
    if (i == NUM) {
        fprintf(stderr, "clients are full, connection is rejected\n");
        return;
    }
    int len = sizeof(struct sockaddr_in);
    clients[i].connfd = accept(lfd, (struct sockaddr*)&clients[i].clientAddr, &len);
    if (clients[i].connfd == -1)
        sys_err("accept clients connection failed");

    clients[i].port = ntohs(clients[i].clientAddr.sin_port);
    inet_ntop(AF_INET, &clients[i].clientAddr.sin_addr.s_addr, clients[i].ip, 16);
    fprintf(stdout, "connect with %s:%d\n", clients[i].ip, clients[i].port);
    if (i > maxIdx)
        maxIdx = i;

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = clients[i].connfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, clients[i].connfd, &ev) == -1)
        sys_err("epoll_ctl add connfd failed");
    
}

void Set_socket() {
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
        sys_err("socket error");
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(lfd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        sys_err("bind error");
    if (listen(lfd, NUM) == -1)
        sys_err("listen error");

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = lfd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev) == -1)
        sys_err("epoll_ctl add lfd error");
    
    InitClients();
}

void InitClients() {
    for (int i = 0; i < NUM; i++)
        clients[i].connfd = -1;
}

void Epoll_create() {
    epfd = epoll_create(NUM);
    if (epfd == -1) 
        sys_err("epoll_create fail");
}

void POST(int connfd, const char* str) {
    char buf[8192] = { 0 };
    strcat(buf, "");
}

void init_POST(int connfd) {    //http format
    char buf[8192] = { 0 };
    strcat(buf, "HTTP/1.1 200 OK\n");
    strcat(buf, "Date: ");
    strcat(buf, __DATE__);
    strcat(buf, "\nContent-Type: text/html; charset=utf-8\r\n\r\n");
    
    strcat(buf, "<html>\n<head>\n<meta charset=\"utf-8\">\n<title>main page</title>\n</head>\n<body>\n\t<h1>标题</h1>\n\t<p>段落。</p>\n</body>\n</html>\n");
    write(connfd, buf, sizeof(buf));
    //fprintf(stdout, "write back complete\n%s\n", buf);
}

void solve_GET(int connfd, char* buf, int size) {
    char tmp[512] = { 0 };
    int idx = 1;
    tmp[0] = buf[0];
    for (; idx < 512; idx++) {
        tmp[idx] = buf[idx];
        if (buf[idx] == '\n' && buf[idx - 1] == '\r')
           break;
    }

    char fileName[256] = { 0 };
    idx = 4;
    while (tmp[idx] != ' ') {
        fileName[idx - 4] = tmp[idx];
        idx++;
    }
    fileName[idx - 4] = 0;
    if (strcmp(fileName, "/") == 0) {
        //fprintf(stdout, "fileName:%s\n", fileName);
        init_POST(connfd);
    }
    else {
        //fprintf(stdout, "fileName:%s\n", fileName);
    }
}