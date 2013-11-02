#include "net.h"
#include "log.h"

#include <stdlib.h>
#include <arpa/inet.h>

void accept_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int, void *);
listener *listener_new(struct event_base* base, struct sockaddr *sa, int socklen, user_callback *cb)
{
    listener * l = (listener *)malloc(sizeof(listener));
    if (NULL == l) {
        mfatal("listener alloc failed!");
        return NULL;
    }

    /* listener */
    struct evconnlistener *listener = evconnlistener_new_bind(base, accept_cb, (void *)l,
            LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
            (struct sockaddr *)sa, socklen);

    if (NULL == listener) {
        mfatal("create evconnlistener failed!");
        free(l);
        return NULL;
    }

    l->cb.type = 'l';
    l->cb.rpc = cb? cb->rpc : NULL;
    l->cb.connect = cb ? cb->connect : NULL;
    l->cb.disconnect = cb ? cb->disconnect : NULL;
    l->l = listener;
    snprintf(l->addrtext, 32, "%s:%d",
            inet_ntoa(((struct sockaddr_in *)sa)->sin_addr),
            ntohs(((struct sockaddr_in *)sa)->sin_port));
    return l;
}

void listener_free(listener *l)
{
    if (l->l)
        evconnlistener_free(l->l);
    free(l);
}
