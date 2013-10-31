#include "net.h"

void gate_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
}

void gate_connect_cb(conn *c, int ok)
{

}

void gate_disconnect_cb(conn *c)
{

}

void gate_cb_init(user_callback *cb)
{
    cb->rpc = gate_rpc_cb;
    cb->connect = gate_connect_cb;
    cb->disconnect = gate_disconnect_cb;
}
