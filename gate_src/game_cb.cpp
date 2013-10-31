
#include "msg_protobuf.h"
#include "cmd.h"
#include "net.h"
#include "log.h"

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[MG_END - MG_BEGIN];

static void game_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        merror("message_head failed!");
        return;
    }
    mdebug("game_rpc_cb cmd:%d", h.cmd);

    if (h.cmd > MG_BEGIN && h.cmd < MG_END) {
        if (cbs[h.cmd - MG_BEGIN]) {
            (*(cbs[h.cmd - MG_BEGIN]))(c, msg, sz);
        }
    } else {
        merror("invalid cmd:%d", h.cmd);
        return;
    }
}

static void game_connect_cb(conn *c)
{
    mdebug("game_connect_cb");
}

static void game_disconnect_cb(conn *c)
{
    mdebug("game_disconnect_cb");
}

void game_cb_init(user_callback *cb)
{
    cb->rpc = game_rpc_cb;
    cb->connect = game_connect_cb;
    cb->disconnect = game_disconnect_cb;
    memset(cbs, 0, sizeof(cb) * (MG_END - MG_BEGIN));
}
