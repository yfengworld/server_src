#include "net.h"
#include "log.h"

#include <stdlib.h>

static conn **freeconns;
static int freetotal;
static int freecurr;
/* lock for connection freelist */
static pthread_mutex_t conn_lock = PTHREAD_MUTEX_INITIALIZER;

void conn_init() {
    freetotal = 200;
    freecurr = 0;
    if (NULL == (freeconns = (conn **)calloc(freetotal, sizeof(conn *)))) {
        merror("connection freelist alloc failed!");
    }
    return;
}

static conn *conn_from_freelist() {
    conn *c;

    pthread_mutex_lock(&conn_lock);
    if (freecurr > 0) {
        c = freeconns[--freecurr];
    } else {
        c = NULL;
    }
    pthread_mutex_unlock(&conn_lock);

    return c;
}

conn *conn_new()
{
    conn *c = conn_from_freelist();
    if (NULL == c) {
        if (!(c = (conn *)malloc(sizeof(conn)))) {
            merror("connection alloc failed!");
            return NULL;
        }
    }
    pthread_mutex_init(&c->lock, NULL);
    c->refcnt = 1;
    return c;
}

#define MAX_CONN_FREELIST 512

static int conn_add_to_freelist(conn *c) {
    int ret = -1;
    pthread_mutex_lock(&conn_lock);
    if (MAX_CONN_FREELIST > freecurr) {
        if (freecurr < freetotal) {
            freeconns[freecurr++] = c;
            ret = 0;
        } else {
            size_t newsize = freetotal * 2;
            conn **new_freeconns = (conn **)realloc(freeconns, sizeof(conn *) * newsize);
            if (new_freeconns) {
                freetotal = newsize;
                freeconns = new_freeconns;
                freeconns[freecurr++] = c;
                ret = 0;
            }
        }
    } else {
        mdebug("reach MAX_CONN_FREELIST:%d", MAX_CONN_FREELIST);
    }
    pthread_mutex_unlock(&conn_lock);
    return ret;
}

void conn_free(conn *c)
{
    pthread_mutex_lock(&c->lock);
    conn_decref_unlock(c);
}

void conn_lock_incref(conn *c)
{
    pthread_mutex_lock(&c->lock);
    ++c->refcnt;
}

void conn_decref_unlock(conn *c)
{
    if (--c->refcnt) {
        pthread_mutex_unlock(&c->lock);
        return;
    }
    if (NULL != c->bev) {
        bufferevent_free(c->bev);
        c->bev = NULL;
    }
    if (0 != conn_add_to_freelist(c)) {
        free(c);
    }
}

void conn_incref(conn *c)
{
    pthread_mutex_lock(&c->lock);
    ++c->refcnt;
    pthread_mutex_unlock(&c->lock);
}

void conn_decref(conn *c)
{
    pthread_mutex_lock(&c->lock);
    return conn_decref_unlock(c);
}

void disconnect(conn *c)
{
    if (c) {
        conn_lock_incref(c);
        if (c->bev) {
            bufferevent_free(c->bev);
            c->bev = NULL;
        }
        conn_decref_unlock(c);
    }
}

int conn_write(conn *c, unsigned char *msg, size_t sz) {
    conn_lock_incref(c);
    if (c->bev && 0 == bufferevent_write(c->bev, msg, sz)) {
        conn_decref_unlock(c);
        return bufferevent_enable(c->bev, EV_WRITE);
    }
    conn_decref_unlock(c);
    return -1;
}
