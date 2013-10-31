#include "net.h"
#include "msg.h"
#include "log.h"

#include <event2/bufferevent.h>
#include <event2/buffer.h>

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
