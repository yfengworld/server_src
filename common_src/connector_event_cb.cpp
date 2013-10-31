#include "msg.h"
#include "net.h"
#include "log.h"

#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* forward declare */
void conn_read_cb(struct bufferevent *, void *);
void conn_write_cb(struct bufferevent *, void *);

static void go_connecting(int fd, short what, void *arg)
{
    conn *c = (conn *)arg;
    connector *cr = (connector *)c->data;
    bufferevent_socket_connect(c->bev, cr->sa, cr->socklen);
}

static void delay_connecting(conn *c)
{
    connector *cr = (connector *)c->data;
    if (NULL == cr->timer) {
        cr->tv.tv_sec = 5;
        cr->tv.tv_usec = 0;
        cr->timer = evtimer_new(c->thread->base, go_connecting, c);
        if (NULL == cr->timer) {
            merror("evtimer_new failed!");
            return;
        }
    }
    evtimer_add(cr->timer, &cr->tv);
}

void connecting_event_cb(struct bufferevent *, short, void *);
static void conn_event_cb2(struct bufferevent *bev, short what, void *arg)
{
    conn *c = (conn *)arg;

    if (what & BEV_EVENT_EOF || what & BEV_EVENT_ERROR) {
        connector *cr = (connector *)c->data;
        if (0 == cr->keep_connect) {
            /* cb */
            user_callback *cb = (user_callback *)c->data;
            if (cb->connect) {
                (*(cb->disconnect))(c);
            }
            return;
        }

        /* free bufferevent */
        bufferevent_free(c->bev);

        /* reconnect */
        c->bev = bufferevent_socket_new(c->thread->base, -1,
                BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
        if (NULL == c->bev) {
            merror("create bufferevent failed!");
            return;
        } else {
            bufferevent_setcb(c->bev, NULL, NULL, connecting_event_cb, c);
            pthread_mutex_lock(&cr->lock);
            cr->state = STATE_NOT_CONNECTED;
            pthread_mutex_unlock(&cr->lock);
            delay_connecting(c);
        }
    }
}

void connecting_event_cb(struct bufferevent *bev, short what, void *arg)
{
    conn *c = (conn *)arg;
    connector *cr = (connector *)c->data;

    if (!(what & BEV_EVENT_CONNECTED)) {
        mdebug("connecting failed!");
        if (0 == cr->keep_connect) {
            user_callback *cb = (user_callback *)c->data;
            if (cb->connect)
                (*(cb->connect))(c, 0);
        } else {
            delay_connecting(c);
        }
    } else {
        user_callback *cb = (user_callback *)c->data;
        if (cb->connect)
            (*(cb->connect))(c, 1);

        minfo("connect %s success!", cr->addrtext);
        pthread_mutex_lock(&cr->lock);
        cr->state = STATE_CONNECTED;
        pthread_mutex_unlock(&cr->lock);

        /* prevent CLOSE_WAIT */
        int fd = bufferevent_getfd(bev);
        linger l;
        l.l_onoff = 1;
        l.l_linger = 0;
        setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&l, sizeof(l));
        bufferevent_setcb(bev, conn_read_cb, conn_write_cb, conn_event_cb2, c);
        bufferevent_enable(bev, EV_READ);
    }
}
