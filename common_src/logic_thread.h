#ifndef LOGIC_THREAD_H_INCLUDED
#define LOGIC_THREAD_H_INCLUDED

#include "net.h"

#include <pthread.h>

typedef struct {
    pthread_t thread;
    user_callback cb;
    struct event_base *base;
    struct event notify_event;
    int notify_receive_fd;
    int notify_send_fd;
} LOGIC_THREAD;

void logic_thread_init(LOGIC_THREAD *logic, user_callback *cb);
int logic_thread_add_rpc_event(LOGIC_THREAD *logic, conn *c, unsigned char *msg, size_t sz);
int logic_thread_add_connect_event(LOGIC_THREAD *logic, conn *c, int ok);
int logic_thread_add_disconnect_event(LOGIC_THREAD *logic, conn *c);

#endif /* LOGIC_THREAD_H_INCLUDED */
