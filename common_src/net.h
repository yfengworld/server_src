#ifndef NET_H_INCLUDED
#define NET_H_INCLUDED

#include "atomic_counter.h"

#include <event2/util.h>
#include <event2/listener.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <event.h>

#include <pthread.h>

struct conn_queue;

typedef struct {
    pthread_t thread_id;
    struct event_base *base;
    struct event notify_event;
    int notify_receive_fd;
    int notify_send_fd;
    struct conn_queue *new_conn_queue;
} LIBEVENT_THREAD;

typedef struct {
    pthread_t thread_id;
    struct event_base *base;
} LIBEVENT_DISPATCHER_THREAD;

typedef struct {
    void *data;                 /* listener or connector */
    void *user;                 /* associate logic object */
    void *arg;                  /* custom data */
#define STATE_NOT_CONNECTED 0
#define STATE_CONNECTED 1
    int state;
    bufferevent *bev;
	struct atomic_counter refcnt;
    pthread_mutex_t lock;
    char addrtext[32];
    LIBEVENT_THREAD *thread;
} conn;

typedef void (*rpc_cb_func)(conn *, unsigned char *, size_t);
typedef void (*connect_cb_func)(conn *, int);
typedef void (*disconnect_cb_func)(conn *);

typedef struct {
    char type;
    rpc_cb_func rpc;
    connect_cb_func connect;
    disconnect_cb_func disconnect;
} user_callback;

typedef struct {
    user_callback cb;
    evconnlistener *l;
    char addrtext[32];
} listener;

typedef struct {
    listener *l;
    char addrtext[32];
} listener_info;

typedef struct {
    user_callback cb;
    int keep_connect;
    conn *c;
    pthread_mutex_t lock;
    struct sockaddr *sa;
    int socklen;
    char addrtext[32];
    /* reconnect timer */
    struct event timer;
} connector;

/* net init */
int net_init();

/* thread clone */
void thread_init(struct event_base *base, int nthreads, pthread_t *th);

/* connection */
void conn_init();
conn *conn_new();
void conn_incref(conn *c);
int conn_decref(conn *c);
void conn_lock(conn *c);
void conn_unlock(conn *c);
void disconnect(conn *c);
void dispatch_conn_new(int fd, char key, void *arg);
int conn_write(conn *c, unsigned char *msg, size_t sz);

/* listener */
listener *listener_new(struct event_base* base,
        struct sockaddr *sa, int socklen, user_callback *cb);
void listener_free(listener *l);

/* 
 * connector
 * kc mean keep connect
 */
connector *connector_new(struct sockaddr *sa, int socklen, int kc, user_callback *cb);
void connector_free(connector *cr);
int connector_write(connector *cr, unsigned char *msg, size_t sz);

#endif /* NET_H_INCLUDED */
