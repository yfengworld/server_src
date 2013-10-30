#include "net.h"
#include "cmd.h"

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[GE_END - GE_BEGIN];

static void user_session_reply_cb(conn *c, unsigned char *msg, size_t sz)
{
    mdebug("user_session_reply_cb");
}

static void gate_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        merror("message_head failed!");
        return;
    }
    mdebug("gate_rpc_cb cmd:%d", h.cmd);

    if (h.cmd > GE_BEGIN && h.cmd < GE_END) {
        if (cbs[h.cmd - GE_BEGIN]) {
            (*(cbs[h.cmd - GE_BEGIN]))(c, msg, sz);
        }
    } else {
        merror("invalid cmd:%d", h.cmd);
        return;
    }
}

static void gate_connect_cb(conn *c)
{
    mdebug("gate_connect_cb");
}

static void gate_disconnect_cb(conn *c)
{
    mdebug("gate_disconnect_cb");
}

void gate_cb_init(user_callback *cb)
{
    cb->rpc = gate_rpc_cb;
    cb->connect = gate_connect_cb;
    cb->disconnect = gate_disconnect_cb;
    memset(cbs, 0, sizeof(cb) * (GE_END - GE_BEGIN));
    cbs[ge_user_session_reply - GE_BEGIN] = user_session_reply_cb;
}
