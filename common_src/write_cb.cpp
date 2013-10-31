#include "net.h"

#include <event2/bufferevent.h>
#include <event2/buffer.h>

void conn_write_cb(struct bufferevent *bev, void *arg)
{
    evbuffer *output = bufferevent_get_output(bev);
    size_t sz = evbuffer_get_length(output);
    if (sz > 0)
        bufferevent_enable(bev, EV_WRITE);
}
