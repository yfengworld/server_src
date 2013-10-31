#include "msg.h"
#include "net.h"
#include "log.h"

#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
