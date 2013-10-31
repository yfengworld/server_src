#include "cmd.h"
#include "net.h"
#include "log.h"

#include <string.h>

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[ME_END - ME_BEGIN];

void game_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    mdebug("game_rpc_cb");
}

void game_connect_cb(conn *c, int ok)
{
    mdebug("game_connect_cb");
}

void game_disconnect_cb(conn *c)
{
    mdebug("game_disconnect_cb");
}

void game_cb_init(user_callback *cb)
{
    cb->rpc = game_rpc_cb;
    cb->connect = game_connect_cb;
    cb->disconnect = game_disconnect_cb;
    memset(cbs, 0, sizeof(cb) * (ME_END - ME_BEGIN));
}
