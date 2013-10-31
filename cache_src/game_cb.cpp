#include "net.h"

void game_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{

}

void game_connect_cb(conn *c, int ok)
{

}

void game_disconnect_cb(conn *c)
{

}

void game_cb_init(user_callback *cb)
{
    cb->rpc = game_rpc_cb;
    cb->connect = game_connect_cb;
    cb->disconnect = game_disconnect_cb;
}
