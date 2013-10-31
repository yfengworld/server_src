#include "net.h"
#include "cmd.h"
#include "log.h"
#include "msg_protobuf.h"

#include "fwd.h"
#include "login.pb.h"
#include "user.h"

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[CL_END - CL_BEGIN];

static void login_request_cb(conn *c, unsigned char *msg, size_t sz)
{
    mdebug("login_request_cb");
    /* account passwd check */
    uint64_t uid = 1;

    user_t *user = user_new(uid);
    if (NULL == user) {
        login_reply lr;
        lr.set_err(1);
        conn_write<login_reply>(c, lc_login_reply, &lr);
        return;
    }

    pthread_rwlock_rdlock(&centers_rwlock);
    if (centers)
    {
        user_login_request ulr;
        ulr.set_uid(uid);
        conn_write<user_login_request>(centers->c, le_user_login_request, &ulr);
        pthread_rwlock_unlock(&centers_rwlock);
        /* add user to map */
        pthread_rwlock_wrlock(&user_mgr->rwlock);
        user_map_t::iterator itr = user_mgr->users_.find(uid);
        if (itr == user_mgr->users_.end()) {
            pthread_mutex_lock(&user->lock);
            user_mgr->users_.insert(std::make_pair(uid, user));
            pthread_mutex_unlock(&user->lock);
        } else {
            user_t *old = itr->second;
            pthread_mutex_lock(&user->lock);
            pthread_mutex_lock(&old->lock);
            itr->second = user;
            pthread_mutex_unlock(&user->lock);
            pthread_mutex_unlock(&old->lock);
        }
        pthread_rwlock_unlock(&user_mgr->rwlock);
    } else {
        user_free(&user);
        pthread_rwlock_unlock(&centers_rwlock);
    }
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
    mdebug("client_connect_cb");
}

void client_disconnect_cb(conn *c)
{
    mdebug("client_disconnect_cb");
    user_t *user = (user_t*)c->user;
    if (NULL != user) {
        pthread_rwlock_wrlock(&user_mgr->rwlock);
        user_map_t::iterator itr = user_mgr->users_.find(user->id);
        if (itr != user_mgr->users_.end()) {
            pthread_mutex_lock(&user->lock);
            user_mgr->users_.erase(itr);
            pthread_mutex_unlock(&user->lock);
        }
        pthread_rwlock_unlock(&user_mgr->rwlock);
    }
}

void client_cb_init(user_callback *cb)
{
    cb->rpc = client_rpc_cb;
    cb->connect = client_connect_cb;
    cb->disconnect = client_disconnect_cb;

    memset(cbs, 0, sizeof(cb) * (CL_END - CL_BEGIN));
    cbs[cl_login_request - CL_BEGIN] = login_request_cb;
}
