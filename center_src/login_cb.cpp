#include "msg_protobuf.h"
#include "cmd.h"
#include "net.h"
#include "log.h"

#include <string.h>

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[LE_END - LE_BEGIN];

static void user_login_request_cb(conn *c, unsigned char *msg, size_t sz)
{
    mdebug("user_login_request_cb");
}

static void login_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        merror("message_head failed!");
        return;
    }
    mdebug("login_rpc_cb cmd:%d", h.cmd);

    if (h.cmd > LE_BEGIN && h.cmd < LE_END) {
        if (cbs[h.cmd - LE_BEGIN]) {
            (*(cbs[h.cmd - LE_BEGIN]))(c, msg, sz);
        }
    } else {
        merror("invalid cmd:%d", h.cmd);
        return;
    }
}

static void login_connect_cb(conn *c, int ok)
{
    mdebug("login_connect_cb");
}

static void login_disconnect_cb(conn *c)
{
    mdebug("login_disconnect_cb");
}

void login_cb_init(user_callback *cb)
{
    cb->rpc = login_rpc_cb;
    cb->connect = login_connect_cb;
    cb->disconnect = login_disconnect_cb;
    memset(cbs, 0, sizeof(cb) * (LE_END - LE_BEGIN));
    cbs[le_user_login_request - LE_BEGIN] = user_login_request_cb;
}
