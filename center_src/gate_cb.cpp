#include "fwd.h"
#include "gate_info.h"
#include "login.pb.h"

#include "msg_protobuf.h"
#include "cmd.h"
#include "net.h"
#include "log.h"

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[GE_END - GE_BEGIN];

static void gate_reg_cb(conn *c, unsigned char *msg, size_t sz)
{
    login::gate_reg r;
    if (0 > msg_body<login::gate_reg>(msg, sz, &r)) {
        merror("msg_body<login::reg> failed!");
        return;
    }

    gate_info_mgr->add_gate_info(c, r.ip().c_str(), r.port());
}

static void user_session_reply_cb(conn *c, unsigned char *msg, size_t sz)
{
    login::user_session_reply r;
    if (0 > msg_body<login::user_session_reply>(msg, sz, &r)) {
        merror("msg_body<login::user_session_reply failed!");
        return;
    }

    login::user_login_reply rr;
    login::error err = r.err();
    rr.set_err(err);
    rr.set_tempid(r.tempid());
    if (login::success == err) {
        char ip[32];
        short port;
        if (0 > gate_info_mgr->get_gate_ip_port(c, ip, &port)) {
            merror("gate_gate_ip_port failed!");
            rr.set_err(login::unknow);
        } else {
            rr.set_uid(r.uid());
            rr.set_sk(r.sk());
            rr.set_gateip(ip);
            rr.set_gateport(port);
        }
    }
    connector_write<login::user_login_reply>(login_, el_user_login_reply, &rr);
}


static void gate_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        merror("message_head failed!");
        return;
    }
    mdebug("gate -> center cmd:%d len:%d flags:%d", h.cmd, h.len, h.flags);

    if (h.cmd > GE_BEGIN && h.cmd < GE_END) {
        if (cbs[h.cmd - GE_BEGIN]) {
            (*(cbs[h.cmd - GE_BEGIN]))(c, msg, sz);
        }
    } else {
        merror("gate -> center invalid cmd:%d len:%d flags:%d", h.cmd, h.len, h.flags);
        return;
    }
}

static void gate_connect_cb(conn *c, int ok)
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
    cbs[ge_gate_reg - GE_BEGIN] = gate_reg_cb;
    cbs[ge_user_session_reply - GE_BEGIN] = user_session_reply_cb;
}
