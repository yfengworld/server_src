#include "fwd.h"
#include "user.h"
#include "center_info.h"
#include "login.pb.h"

#include "msg_protobuf.h"
#include "cmd.h"
#include "net.h"
#include "log.h"

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[EL_END - EL_BEGIN];

static void center_reg_cb(conn *c, unsigned char *msg, size_t sz)
{
    login::center_reg r;
    if (0 > msg_body<login::center_reg>(msg, sz, &r)) {
        merror("msg_body<login::center_reg> failed!");
        return;
    }
    center_info_mgr->add_center_info(r.id(), c);
}

static void user_login_reply_cb(conn *c, unsigned char *msg, size_t sz)
{
    login::user_login_reply r;
    msg_body<login::user_login_reply>(msg, sz, &r);

    login::error err = r.err();
    int tempid = r.tempid();
    user_t *user = user_mgr->get_user_incref(tempid);
    if (NULL == user) {
        mwarn("user tempid:%d not exist when user_login_reply_cb reach!", tempid);
        return;
    }

    login::login_reply rr;
    rr.set_err(err);
    if (login::success == err) {
        rr.set_uid(r.uid());
        rr.set_sk(r.sk());
        rr.set_gateip(r.gateip());
        rr.set_gateport(r.gateport());
    } 
    conn_write<login::login_reply>(user->c, lc_login_reply, &rr);
}

static void center_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        merror("message_head failed!");
        return;
    }
    mdebug("center -> login cmd:%d len:%d flags:%d", h.cmd, h.len, h.flags);

    if (h.cmd > EL_BEGIN && h.cmd < EL_END) {
        if (cbs[h.cmd - EL_BEGIN]) {
            if (cbs[h.cmd - EL_BEGIN]) {
                (*(cbs[h.cmd - EL_BEGIN]))(c, msg, sz);
            }
        } else {
            merror("center -> login invalid cmd:%d len:%d flags:%d", h.cmd, h.len, h.flags);
            return;
        }
    }
}

static void center_connect_cb(conn *c, int ok)
{
    mdebug("center_connect_cb");
}

static void center_disconnect_cb(conn *c)
{
    mdebug("center_disconnect_cb");
    center_info_mgr->del_center_info((int)(long)(c->arg));
}

void center_cb_init(user_callback *cb)
{
    cb->rpc = center_rpc_cb;
    cb->connect = center_connect_cb;
    cb->disconnect = center_disconnect_cb;

    memset(cbs, 0, sizeof(cb) * (EL_END - EL_BEGIN));
    cbs[el_user_login_reply - EL_BEGIN] = user_login_reply_cb;
    cbs[el_center_reg - EL_BEGIN] = center_reg_cb;
}
