#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <iostream>
#include "locker.h"
#include "thread_pool.h"
// #include "http_conn.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include "epoll_frame.h"

void run() {
    epoll_frame ef(1024);
    ef.set_timer(3);
    ef.dispatch();
}

#endif 