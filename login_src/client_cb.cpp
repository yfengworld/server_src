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
    uint64_t *uid = (uint64_t *)arg;
    user_t *user = user_mgr->get_user_incref(*uid);
    if (user)
    {
        user->decref_unlock();
        user->decref();
    }
    free(uid);
}

static void login_request_cb(conn *c, unsigned char *msg, size_t sz)
{
    mdebug("login_request_cb");

    /* TODO account passwd check */
    uint64_t uid = 1;

    int guid = user_t::get_guid();
    user_t *user = new (std::nothrow) user_t(guid);
    if (NULL == user) {
        login_reply lr;
        lr.set_err(99);
        conn_write<login_reply>(c, lc_login_reply, &lr);
        return;
    }

    if (0 > user_mgr->add_user(user)) {
        login_reply lr;
        lr.set_err(99);
        conn_write<login_reply>(c, lc_login_reply, &lr);
        return;
    }

    user->set_conn(c);

    int error = 0;

    do
    {
        /*
        uint64_t *data = (uint64_t *)malloc(sizeof(uint64_t));
        if (NULL == data) {
            error = 99;
            break;
            
        }

        user->timer = evtimer_new(c->thread->base, expire_timer_cb, data);
        if (NULL == user->timer) {
            free(data);
            error = 99;
            break;
        }

        user->tv.tv_sec = 5;
        user->tv.tv_usec = 0;
        if (evtimer_add(user->timer, &user->tv) < 0) {
            free(data);
            error = 99;
            break;
        }
        */

        /* tell center */
        pthread_rwlock_rdlock(&centers_rwlock);
        if (centers)
        {
            user_login_request ulr;
            ulr.set_uid(uid);
            conn_write<user_login_request>(centers->c, le_user_login_request, &ulr);
        }
        pthread_rwlock_unlock(&centers_rwlock);
        return;
    } while (0);

    user_mgr->del_user(user);
    user->decref();

    login_reply lr;
    lr.set_err(error);
    conn_write<login_reply>(c, lc_login_reply, &lr);
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
