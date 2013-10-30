#include "net.h"
#include "cmd.h"

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[AG_END - AG_BEGIN];

static void cache_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        merror("message_head failed!");
        return;
    }
    mdebug("cache_rpc_cb cmd:%d", h.cmd);

    if (h.cmd > AG_BEGIN && h.cmd < AG_END) {
        if (cbs[h.cmd - AG_BEGIN]) {
            (*(cbs[h.cmd - AG_BEGIN]))(c, msg, sz);
        }
    } else {
        merror("invalid cmd:%d", h.cmd);
        return;
    }
}

static void cache_connect_cb(conn *c)
{
    mdebug("cache_connect_cb");
}

static void cache_disconnect_cb(conn *c)
{
    mdebug("cache_disconnect_cb");
}

void cache_cb_init(user_callback *cb)
{
    cb->rpc = cache_rpc_cb;
    cb->connect = cache_connect_cb;
    cb->disconnect = cache_disconnect_cb;
    memset(cbs, 0, sizeof(cb) * (AG_END - AG_BEGIN));
}
