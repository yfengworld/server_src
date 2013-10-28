#include "net.h"

void gate_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    message_head(msg, sz, &h);
    mdebug("magic:%d len:%d cmd:%d flags:%d", h.magic, h.len, h.cmd, h.flags); 
}

void gate_connect_cb(conn *c)
{

}

void gate_disconnect_cb(conn *c)
{

}
