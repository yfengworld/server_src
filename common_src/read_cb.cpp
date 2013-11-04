#include "net.h"
#include "msg.h"
#include "log.h"

#include <event2/bufferevent.h>
#include <event2/buffer.h>

void conn_read_cb(struct bufferevent *bev, void *arg)
{
    conn *c = (conn *)arg;
    user_callback *cb = (user_callback *)(c->data);

    struct evbuffer* input = bufferevent_get_input(bev);
    size_t total_len = evbuffer_get_length(input);

    while (1)
    {
        if (total_len < MSG_HEAD_SIZE) {
            goto conti;
        }
        else {
            unsigned char *buffer = evbuffer_pullup(input, MSG_HEAD_SIZE);
            if (NULL == buffer)
            {
                merror("evbuffer_pullup MSG_HEAD_SIZE failed!");
                goto err;
            }

            unsigned short *cur = (unsigned short *)buffer;
            unsigned int len = ntohs(*(unsigned int *)cur);
            cur += 2;

            if (MSG_MAX_SIZE < len)
            {
                merror("len:%d > MSG_MAX_SIZE!", len);
                goto err;
            }

            if (total_len < MSG_HEAD_SIZE + len)
                goto conti;

            unsigned short cmd = ntohs(*(unsigned short *)cur++);
            unsigned short flags = ntohs(*(unsigned short *)cur);

            size_t msg_len = MSG_HEAD_SIZE + len;
            buffer = evbuffer_pullup(input, msg_len);
            if (NULL == buffer)
            {
                merror("evbuffer_pullup msg_len failed!");
                goto err;
            }

            /* TODO frequency limit */

            /* callback */
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
    if (cb->disconnect) {
        (*(cb->disconnect))(c);
    }
    conn_decref(c);
    return;

conti:
    bufferevent_enable(bev, EV_READ);
    return;
}
