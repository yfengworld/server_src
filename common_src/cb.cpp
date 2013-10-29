#include "net.h"
#include "msg.h"
#include "cmd.h"
#include "test.pb.h"

#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void accept_cb(struct evconnlistener *l, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg)
{
    listener_info *li = (listener_info *)malloc(sizeof(listener_info));
    if (NULL == li) {
        merror("listener_info alloc failed!");
        return;
    }
    li->l = (listener *)arg;
    snprintf(li->addrtext, 32, "%s:%d",
            inet_ntoa(((struct sockaddr_in *)sa)->sin_addr),
                ntohs(((struct sockaddr_in *)sa)->sin_port));

    dispatch_conn_new(fd, 'c', li);
}

void conn_read_cb(struct bufferevent *bev, void *arg)
{
    size_t total_len;

    struct evbuffer* input = bufferevent_get_input(bev);
    total_len = evbuffer_get_length(input);

    while (1)
    {
        if (total_len < MSG_HEAD_SIZE) {
            goto conti;
        }
        else {
            unsigned char *buffer;
            unsigned short *cur;
            unsigned short magic_number, len, cmd, flags;

            buffer = evbuffer_pullup(input, MSG_HEAD_SIZE);
            if (NULL == buffer)
            {
                merror("evbuffer_pullup MSG_HEAD_SIZE failed!");
                goto err;
            }

            cur = (unsigned short *)buffer;
            magic_number = ntohs(*(unsigned short *)cur++);
            if (MAGIC_NUMBER != magic_number)
            {
                merror("magic_number error!");
                goto err;
            }

            len = ntohs(*(unsigned short *)cur++);

            if (MSG_MAX_SIZE < len)
            {
                merror("len:%d > MSG_MAX_SIZE!", len);
                goto err;
            }

            if (total_len < MSG_HEAD_SIZE + len)
                goto conti;

            cmd = ntohs(*(unsigned short *)cur++);
            flags = ntohs(*(unsigned short *)cur);

            size_t msg_len = MSG_HEAD_SIZE + len;
            buffer = evbuffer_pullup(input, msg_len);
            if (NULL == buffer)
            {
                merror("evbuffer_pullup msg_len failed!");
                goto err;
            }

            /* TODO frequency limit */

            /* callback */
            conn *c = (conn *)arg;
            user_callback *cb = (user_callback *)(c->data);
            if (cb->rpc)
                (*(cb->rpc))(c, buffer, msg_len);

            if (evbuffer_drain(input, msg_len) < 0)
            {
                merror("evbuffer_drain failed!");
                goto err;
            }

            total_len -= msg_len;
        }
    }
    return;

err:
    mdebug("close sockect!");
    bufferevent_free(bev);
    return;
conti:
    bufferevent_enable(bev, EV_READ);
    return;
}

void conn_write_cb(struct bufferevent *bev, void *arg)
{
    evbuffer *output = bufferevent_get_output(bev);
    size_t sz = evbuffer_get_length(output);
    if (sz > 0)
        bufferevent_enable(bev, EV_WRITE);
}

void conn_event_cb(struct bufferevent *bev, short what, void *arg)
{
    conn *c = (conn *)arg;

    if (what & BEV_EVENT_EOF || what & BEV_EVENT_ERROR) {
        user_callback *cb = (user_callback *)c->data;
        if (cb->disconnect) {
            (*(cb->disconnect))(c);
        }
    }
}

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
        /* cb */
        user_callback *cb = (user_callback *)c->data;
        if (cb->disconnect) {
            (*(cb->disconnect))(c);
        }

        /* reconnect */
        bufferevent_free(c->bev);
        c->bev = bufferevent_socket_new(c->thread->base, -1,
                BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
        if (NULL == c->bev) {
            merror("create bufferevent failed!");
            return;
        } else {
            bufferevent_setcb(c->bev, NULL, NULL, connecting_event_cb, c);
            connector *cr = (connector *)c->data;
            cr->state = STATE_NOT_CONNECTED;
            delay_connecting(c);
        }
    }
}

void connecting_event_cb(struct bufferevent *bev, short what, void *arg)
{
    conn *c = (conn *)arg;

    if (!(what & BEV_EVENT_CONNECTED)) {
        mdebug("connecting failed!");
        delay_connecting(c);
    } else {
        user_callback *cb = (user_callback *)c->data;
        if (cb->connect)
            (*(cb->connect))(c);

        connector *cr = (connector *)c->data;
        minfo("connect %s success!", cr->addrtext);
        cr->state = STATE_CONNECTED;
        evutil_socket_t fd = bufferevent_getfd(bev);
        linger l;
        l.l_onoff = 1;
        l.l_linger = 0;
        setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&l, sizeof(l));
        bufferevent_setcb(bev, conn_read_cb, conn_write_cb, conn_event_cb2, c);
        bufferevent_enable(bev, EV_READ);
    }
}
