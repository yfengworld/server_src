#include "net.h"
#include "cmd.h"
#include "test.pb.h"

void server_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
}

void server_connect_cb(conn *c)
{
    mdebug("server_connect_cb");
    conn_write(c, ce_test);
}

void server_disconnect_cb(conn *c)
{
    mdebug("server_disconnect_cb");
}
