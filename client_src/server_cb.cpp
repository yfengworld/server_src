#include "fwd.h"
#include "login.pb.h"

#include "msg_protobuf.h"
#include "cmd.h"
#include "logic_thread.h"
#include "net.h"
#include "log.h"

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[SC_END - SC_BEGIN];

static void logic_server_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        return;
    }
}

static void logic_server_connect_cb(conn *c)
{
    mdebug("logic_server_connect_cb");
    login_request lr;
    lr.set_account("abc");
    lr.set_passwd("xxx");
    conn_write<login_request>(c, cl_login_request, &lr);
}

static void logic_server_disconnect_cb(conn *c)
{
    mdebug("logic_server_disconnect_cb");
}

static void server_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    logic_thread_add_rpc_event(&logic, c, msg, sz);
}

static void server_connect_cb(conn *c)
{
    logic_thread_add_connect_event(&logic, c);
}

static void server_disconnect_cb(conn *c)
{
    logic_thread_add_disconnect_event(&logic, c);
}

void server_cb_init(user_callback *cb, user_callback *cb2)
{
    cb->rpc = server_rpc_cb;
    cb->connect = server_connect_cb;
    cb->disconnect = server_disconnect_cb;
    cb2->rpc = logic_server_rpc_cb;
    cb2->connect = logic_server_connect_cb;
    cb2->disconnect = logic_server_disconnect_cb;
    memset(cbs, 0, sizeof(cb) * (SC_END - SC_BEGIN));
}
