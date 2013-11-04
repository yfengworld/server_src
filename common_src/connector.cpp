#include "net.h"
#include "log.h"

#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

connector *connector_new(struct sockaddr *sa, int socklen, int kc, user_callback *cb)
{
    connector *cr = (connector *)malloc(sizeof(connector));
    if (NULL == cr) {
        mfatal("connector alloc failed!");
        return NULL;
    }

    cr->sa = (struct sockaddr *)malloc(socklen);
    if (NULL == cr->sa) {
        mfatal("sockaddr alloc failed!");
        free(cr);
        return NULL;
    }

    cr->keep_connect = kc;

    cr->cb.type = 'c';
    cr->cb.rpc = cb ? cb->rpc : NULL;
    cr->cb.connect = cb ? cb->connect : NULL;
    cr->cb.disconnect = cb ? cb->disconnect : NULL;

    cr->c = NULL;

    pthread_mutex_init(&cr->lock, NULL);

    memcpy(cr->sa, sa, socklen);
    cr->socklen = socklen;
    snprintf(cr->addrtext, 32, "%s:%d",
            inet_ntoa(((struct sockaddr_in *)(cr->sa))->sin_addr),
            ntohs(((struct sockaddr_in *)(cr->sa))->sin_port));
    dispatch_conn_new(-1, 't', cr);
    return cr;
}

void connector_free(connector *cr)
{
    free(cr->sa);
    free(cr);
}

int connector_write(connector *cr, unsigned char *msg, size_t sz)
{
    int ret = -1;
    pthread_mutex_lock(&cr->lock);
    if (cr->c) {
        conn_lock_incref(cr->c);
        if (cr->c->state == STATE_CONNECTED && cr->c->bev) {
            if (0 == bufferevent_write(cr->c->bev, msg, sz)) {
                ret = bufferevent_enable(cr->c->bev, EV_WRITE);
            }
        }
        conn_decref_unlock(cr->c);
    }
    pthread_mutex_unlock(&cr->lock);

    return ret;
}
