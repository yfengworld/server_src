#include "net.h"
#include "cmd.h"
#include "log.h"
#include "msg_protobuf.h"

#include "fwd.h"
#include "login.pb.h"
#include "user.h"

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[CL_END - CL_BEGIN];

static void expire_timer_cb(int fd, short what, void *arg)
{
    mdebug("expire_timer_cb");
    long id = (long)arg;
    user_t *user = user_mgr->get_user_incref((int)id);
    if (user)
    {
        user->decref_unlock();
        user->decref();
    }
}

static void login_request_cb(conn *c, unsigned char *msg, size_t sz)
{
    mdebug("login_request_cb");

    uint64_t uid = 1;

    /* TODO account passwd check */
    if (0) {
        login::login_reply lr;
        lr.set_err(login::auth);
        conn_write<login::login_reply>(c, lc_login_reply, &lr);
        return;
    }

    int guid = user_t::get_guid();
    user_t *user = new (std::nothrow) user_t(guid);
    if (NULL == user) {
        login::login_reply lr;
        lr.set_err(login::unknow);
        conn_write<login::login_reply>(c, lc_login_reply, &lr);
        return;
    }

    if (0 > user_mgr->add_user(user)) {
        login::login_reply lr;
        lr.set_err(login::unknow);
        conn_write<login::login_reply>(c, lc_login_reply, &lr);
        return;
    }

    user->set_conn(c);

    /* start expire timer */
    struct event *timer = user->get_expire_timer();
    struct timeval tv = {5, 0};
    evtimer_set(timer, expire_timer_cb, (void *)guid);
    event_base_set(c->thread->base, timer);
    if (0 > evtimer_add(timer, &tv)) {
        user_mgr->del_user(user);
        user->decref();

        login::login_reply lr;
        lr.set_err(login::unknow);
        conn_write<login::login_reply>(c, lc_login_reply, &lr);
        return;
    }

    /* tell center */
    pthread_rwlock_rdlock(&centers_rwlock);
    if (centers)
    {
        login::user_login_request ulr;
        ulr.set_uid(uid);
        conn_write<login::user_login_request>(centers->c, le_user_login_request, &ulr);
    }
    pthread_rwlock_unlock(&centers_rwlock);
}

void client_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        merror("message_head failed!");
        /* close connection */
        disconnect(c);
        return;
    }
    mdebug("client_rpc_cb cmd:%d", h.cmd);

    if (h.cmd > CL_BEGIN && h.cmd < CL_END) {
        if (cbs[h.cmd - CL_BEGIN]) {
            (*(cbs[h.cmd - CL_BEGIN]))(c, msg, sz);
        }
    } else {
        merror("invalid cmd:%d", h.cmd);
        /* close connection */
        disconnect(c);
        return;
    }
}

void client_connect_cb(conn *c, int ok)
{
    mdebug("client_connect_cb ok:%d", ok);
}

void client_disconnect_cb(conn *c)
{
    mdebug("client_disconnect_cb");
    conn_lock_incref(c);
    user_t* user = (user_t *)c->user;
    user_mgr->del_user(user);
    user->decref();
    conn_decref_unlock(c);
}

void client_cb_init(user_callback *cb)
{
    cb->rpc = client_rpc_cb;
    cb->connect = client_connect_cb;
    cb->disconnect = client_disconnect_cb;

    memset(cbs, 0, sizeof(cb) * (CL_END - CL_BEGIN));
    cbs[cl_login_request - CL_BEGIN] = login_request_cb;
}
