#include "fwd.h"
#include "login.pb.h"

#include "msg_protobuf.h"
#include "cmd.h"
#include "net.h"
#include "log.h"

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[EG_END - EG_BEGIN];

static void user_session_request_cb(conn *c, unsigned char *msg, size_t sz)
{
    mdebug("user_session_request_cb");

    login::user_session_request usr;
    msg_body<login::user_session_request>(msg, sz, &usr);

    login::error err = login::success;
    user_t *user = user_mgr->get_user_incref(usr.uid());
    if (NULL == user) {
        if (0 > user_mgr->add_user(usr.uid())) {
            err = login::unknow;
        } else {
            err = login::success;
        }
    } else {
        strncpy(user->sk, usr.sk().c_str(), 32);
        user->sk[31] = '\0';
        if (user->c) {
            disconnect(user->c);
            conn_decref(user->c);
            user->c = NULL;
        }
        err = login::success;
    }
    login::user_session_reply r;
    r.set_err(err);
    if (err != login::success) {
        r.set_tempid(usr.tempid());
    } else {
        r.set_tempid(usr.tempid());
        r.set_uid(usr.uid());
        r.set_sk(usr.sk());
    }
    conn_write<login::user_session_reply>(c, ge_user_session_reply, &r);
}

static void center_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        merror("message_head failed!");
        return;
    }
    mdebug("center_rpc_cb cmd:%d", h.cmd);

    if (h.cmd > EG_BEGIN && h.cmd < EG_END) {
        if (cbs[h.cmd - EG_BEGIN]) {
            (*(cbs[h.cmd - EG_BEGIN]))(c, msg, sz);
        }
    } else {
        merror("invalid cmd:%d", h.cmd);
        return;
    }
}

static void center_connect_cb(conn *c, int ok)
{
    mdebug("center_connect_cb");
    login::gate_reg r;
    r.set_ip(gate_ip.c_str());
    r.set_port(gate_port);
    int ret = connector_write<login::gate_reg>(center, ge_gate_reg, &r);
    mdebug("connector_write ret:%d", ret);
}

static void center_disconnect_cb(conn *c)
{
    mdebug("center_disconnect_cb");
}

void center_cb_init(user_callback *cb)
{
    cb->rpc = center_rpc_cb;
    cb->connect = center_connect_cb;
    cb->disconnect = center_disconnect_cb;
    memset(cbs, 0, sizeof(cb) * (EG_END - EG_BEGIN));
    cbs[eg_user_session_request - EG_BEGIN] = user_session_request_cb;
}
