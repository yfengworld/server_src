#include "net.h"
#include "cmd.h"
#include "log.h"
#include "msg_protobuf.h"

#include "fwd.h"
#include "login.pb.h"
#include "user.h"

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[CL_END - CL_BEGIN];

static void expire_user(user_t *user)
{
    pthread_rwlock_wrlock(&user_mgr->rwlock);
    pthread_mutex_lock(&user->lock);
    user_map_t::iterator itr = user_mgr->users_.find(user->id);
    if (itr != user_mgr->users_.end()) {
        user_mgr->users_.erase(itr);
    }
    if (user->c) {
        pthread_mutex_lock(&user->c->lock);
        user->c->user = NULL;
        pthread_mutex_unlock(&user->c->lock);
        user->c = NULL;
    }
    pthread_mutex_unlock(&user->lock);
    pthread_rwlock_unlock(&user_mgr->rwlock);
}

static void expire_timer_cb(int fd, short what, void *arg)
{
    uint64_t *uid = (uint64_t *)arg;
    pthread_rwlock_wrlock(&user_mgr->rwlock);
    user_map_t::iterator itr = user_mgr->users_.find(*uid);
    if (itr != user_mgr->users_.end()) {
        user_t *user = itr->second;
        pthread_mutex_lock(&user->lock);
        if (user->c) {
            pthread_mutex_lock(&user->c->lock);
            user->c->user = NULL;
            pthread_mutex_unlock(&user->c->lock);
            user->c = NULL;
        }
        pthread_mutex_unlock(&user->lock);
    }
    pthread_rwlock_unlock(&user_mgr->rwlock);
    free(uid);
}

static void login_request_cb(conn *c, unsigned char *msg, size_t sz)
{
    mdebug("login_request_cb");

    /* TODO account passwd check */
    uint64_t uid = 1;

    user_t *user = user_new(uid);
    if (NULL == user) {
        login_reply lr;
        lr.set_err(1);
        conn_write<login_reply>(c, lc_login_reply, &lr);
        return;
    }

    /* add new user to map */
    int add_res = 0;
    pthread_rwlock_wrlock(&user_mgr->rwlock);
    user_map_t::iterator itr = user_mgr->users_.find(uid);
    if (itr == user_mgr->users_.end()) {
        pthread_mutex_lock(&user->lock);
        user_mgr->users_.insert(std::make_pair(uid, user));
        user->c = c;
        pthread_mutex_unlock(&user->lock);
        pthread_mutex_lock(&c->lock);
        c->user = user;
        pthread_mutex_unlock(&c->lock);
        add_res = 1;
    }
    pthread_rwlock_unlock(&user_mgr->rwlock);

    if (0 == add_res) {
        login_reply lr;
        lr.set_err(1);
        conn_write<login_reply>(c, lc_login_reply, &lr);
        return;
    }

    /* start expire timer */
    uint64_t *data = (uint64_t *)malloc(sizeof(uint64_t));
    if (NULL == data) {
        login_reply lr;
        lr.set_err(2);
        conn_write<login_reply>(c, lc_login_reply, &lr);
        expire_user(user);
        return;
    }

    user->timer = evtimer_new(c->thread->base, expire_timer_cb, data);
    if (NULL == user->timer) {
        free(data);
        login_reply lr;
        lr.set_err(3);
        conn_write<login_reply>(c, lc_login_reply, &lr);
        expire_user(user);
        return;
    }

    user->tv.tv_sec = 5;
    user->tv.tv_usec = 0;
    if (evtimer_add(user->timer, &user->tv) < 0) {
        free(data);
        /* TODO free timer */
        login_reply lr;
        lr.set_err(4);
        conn_write<login_reply>(c, lc_login_reply, &lr);
        expire_user(user);
        return;
    }

    /* tell center */
    pthread_rwlock_rdlock(&centers_rwlock);
    if (centers)
    {
        user_login_request ulr;
        ulr.set_uid(uid);
        conn_write<user_login_request>(centers->c, le_user_login_request, &ulr);
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
    pthread_mutex_lock(&c->lock);
    user_t *user = (user_t*)c->user;
    if (NULL != user) {
        pthread_rwlock_wrlock(&user_mgr->rwlock);
        user_map_t::iterator itr = user_mgr->users_.find(user->id);
        if (itr != user_mgr->users_.end()) {
            pthread_mutex_lock(&user->lock);
            user_mgr->users_.erase(itr);
            pthread_mutex_unlock(&user->lock);
            user_free(&user);
            c->user = NULL;
        }
        pthread_rwlock_unlock(&user_mgr->rwlock);
    }
    pthread_mutex_unlock(&c->lock);
    conn_free(c);
}

void client_cb_init(user_callback *cb)
{
    cb->rpc = client_rpc_cb;
    cb->connect = client_connect_cb;
    cb->disconnect = client_disconnect_cb;

    memset(cbs, 0, sizeof(cb) * (CL_END - CL_BEGIN));
    cbs[cl_login_request - CL_BEGIN] = login_request_cb;
}
