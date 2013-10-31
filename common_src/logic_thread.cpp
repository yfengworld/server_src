#include "logic_thread.h"
#include "log.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/************************************* logic event queue *************************************/

#define STACK_ALLOC_THRESHOLD 24

typedef struct logic_event_item LE_ITEM;
struct logic_event_item {
    LE_ITEM *next;
    char type; /* 'r' = rpc 'c' = connect 'd' = disconnect */
    conn *c;
    union {
        struct {
            char sub_type; /* 's' = stack alloc 'h' = heap alloc */
            struct {
                unsigned char smsg[STACK_ALLOC_THRESHOLD];
                int ssz;
            };
            struct {
                unsigned char *msg;
                int sz;
            };
        };
    };
};

typedef struct logic_event_queue LEQ;
struct logic_event_queue {
    LE_ITEM *head;
    LE_ITEM *tail;
    pthread_mutex_t lock;
};

static LEQ sleq;

static void leq_init(LEQ *leq) {
    pthread_mutex_init(&leq->lock, NULL);
    leq->head = NULL;
    leq->tail = NULL;
}

static void leq_push(LEQ *leq, LE_ITEM *item) {
    item->next = NULL;

    pthread_mutex_lock(&leq->lock);
    if (NULL == leq->tail)
        leq->head = item;
    else
        leq->tail->next = item;
    leq->tail = item;
    pthread_mutex_unlock(&leq->lock);
}

static LE_ITEM *leq_pop(LEQ *leq) {
    LE_ITEM *item;

    pthread_mutex_lock(&leq->lock);
    item = leq->head;
    if (NULL != item) {
        leq->head = item->next;
        if (NULL == leq->head)
            leq->tail = NULL;
    }
    pthread_mutex_unlock(&leq->lock);

    return item;
}

/************************************* logic thread *************************************/

static void exec_logic_event(LOGIC_THREAD *logic, LE_ITEM *item)
{
    if ('r' == item->type) {
        if (logic->cb.rpc) {
            if ('s' == item->sub_type) {
                (*(logic->cb.rpc))(item->c, item->smsg, item->ssz);
                free(item->smsg);
            } else if ('h' == item->sub_type) {
                (*(logic->cb.rpc))(item->c, item->msg, item->sz);
            } else {
                mfatal("invalid sub type!");
            }
        }
    } else if ('c' == item->type) {
        if (logic->cb.connect) {
            (*(logic->cb.connect))(item->c);
        }
    } else if ('d' == item->type) {
        if (logic->cb.disconnect) {
            (*(logic->cb.disconnect))(item->c);
        }
    } else {
        mfatal("invalid logic event type!");
    }
    free(item);
}

static void thread_logic_process(int fd, short which, void *arg)
{
    LOGIC_THREAD *logic = (LOGIC_THREAD *)arg;
    LE_ITEM *item;
    char buf[1];

    if (read(fd, buf, 1) != 1)
        merror("can't read from libevent pipe!");

    switch(buf[0]) {
    case 'd': {
            item = leq_pop(&sleq);
            if (NULL != item) {
                exec_logic_event(logic, item);
            }
        }
        break;
    }
}

static void *logic_thread_func(void *arg)
{
    LOGIC_THREAD *logic = (LOGIC_THREAD *)arg;
    event_base_dispatch(logic->base);
    return NULL;
}

void logic_thread_init(LOGIC_THREAD *logic, user_callback *cb)
{
    logic->base = event_base_new();
    if (NULL == logic->base) {
        mfatal("create logic thread event_base failed!");
        exit(1);
    }

    logic->cb = *cb;
    leq_init(&sleq);

    int fds[2];
    if (pipe(fds)) {
        mfatal("can't create notify pipe!");
        exit(1);
    }
    logic->notify_receive_fd = fds[0];
    logic->notify_send_fd = fds[1];

    event_set(&logic->notify_event, logic->notify_receive_fd,
            EV_READ | EV_PERSIST, thread_logic_process, logic);
    event_base_set(logic->base, &logic->notify_event);

    if (event_add(&logic->notify_event, 0) == -1) {
        mfatal("can't monitor libevent notify pipe!");
        exit(1);
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    int ret = pthread_create(&logic->thread, &attr, logic_thread_func, logic);
    if (0 != ret) {
        mfatal("can't create logic thread:%s!", strerror(ret));
        exit(1);
    }
}

int logic_thread_add_rpc_event(LOGIC_THREAD *logic, conn *c, unsigned char *msg, size_t sz)
{
    LE_ITEM *item = (LE_ITEM *)malloc(sizeof(LE_ITEM));
    if (NULL == item) {
        merror("LE_ITEM alloc failed!");
        return -1;
    }

    item->type = 'r';
    item->c = c;

    if (sz > STACK_ALLOC_THRESHOLD) {
        item->msg = (unsigned char *)malloc(sz);
        if (NULL == item->msg) {
            merror("msg alloc failed!");
            free(item);
            return -1;
        }
        item->sub_type = 'h';
        memcpy(item->msg, msg, sz);
        item->sz = sz;
    } else {
        item->sub_type = 's';
        memcpy(item->smsg, msg, sz);
        item->ssz = sz;
    }

    leq_push(&sleq, item);
    
    char buf[1];
    buf[0] = 'd';
    if (write(logic->notify_send_fd, buf, 1) != 1) {
        merror("writing to thread notify pipe failed!");
        return -1;
    }
    return 0;
}

int logic_thread_add_connect_event(LOGIC_THREAD *logic, conn *c)
{
    LE_ITEM *item = (LE_ITEM *)malloc(sizeof(LE_ITEM));
    if (NULL == item) {
        merror("LE_ITEM alloc failed!");
        return -1;
    }

    item->type = 'c';
    item->c = c;
    leq_push(&sleq, item);

    char buf[1];
    buf[0] = 'd';
    if (write(logic->notify_send_fd, buf, 1) != 1) {
        merror("writing to thread notify pipe failed!");
        return -1;
    }
    return 0;
}

int logic_thread_add_disconnect_event(LOGIC_THREAD *logic, conn *c)
{
    LE_ITEM *item = (LE_ITEM *)malloc(sizeof(LE_ITEM));
    if (NULL == item) {
        merror("LE_ITEM alloc failed!");
        return -1;
    }

    item->type = 'd';
    item->c = c;
    leq_push(&sleq, item);

    char buf[1];
    buf[0] = 'd';
    if (write(logic->notify_send_fd, buf, 1) != 1) {
        merror("writing to thread notify pipe failed!");
        return -1;
    }
    return 0;
}
