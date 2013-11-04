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
    mdebug("conn_event_cb what:%d", what);

    if (what & BEV_EVENT_EOF || what & BEV_EVENT_ERROR) {
        int ret = 0;
        conn_lock(c);
        if (STATE_CONNECTED == c->state) {
            c->state == STATE_NOT_CONNECTED;
            ret = 1;
        }
        conn_unlock(c);
        if (1 == ret) {
            user_callback *cb = (user_callback *)c->data;
            if (cb->disconnect) {
                (*(cb->disconnect))(c);
            }
        }
        conn_decref(c);
    }
}
