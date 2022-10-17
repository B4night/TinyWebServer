#ifndef _EPOLL_FRAME_H
#define _EPOLL_FRAME_H

void Epoll_create();
void init_clients();
void set_socket();
void epfd_add(int event, struct my_events* m_ev);
void epfd_del(struct my_events* m_ev);
void lfd_cb(int, int, void*);
void client_recv_cb(int, int, void*);
void client_send_cb(int, int, void*);
void Epoll_loop();

#endif