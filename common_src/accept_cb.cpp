#include "net.h"
#include "log.h"

#include <stdlib.h>
#include <arpa/inet.h>

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
