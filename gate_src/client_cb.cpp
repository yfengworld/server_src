#include "fwd.h"
#include "login.pb.h"

#include "msg_protobuf.h"
#include "cmd.h"
#include "net.h"
#include "log.h"

typedef void (*cb)(conn *, unsigned char *msg, size_t sz);
static cb cbs[CG_END - CG_BEGIN];

static void connect_request_cb(conn *c, unsigned char *msg, size_t sz)
{
    mdebug("user_session_cb");

    login::connect_request cr;
    
    uint64_t uid = cr.uid();
    std::string sk = cr.sk();

    login::error err = login::success;

    do {
        user_t *user = user_mgr->get_user_incref(uid);
        if (NULL == user) {
            err = login::unknow;
            break;
        }

        if (0 != strncmp(sk.c_str(), user->sk, 32)) {
            err = login::auth;
            break;
        }

        user_lock(user);
        /* replace */
        if (user->c) {
            disconnect(user->c);
            conn_decref(user->c);
        }
        user->c = c;
        conn_incref(user->c);
        user_unlock(user);
        
    } while (0);

    login::connect_reply r;
    r.set_err(err);
    conn_write<login::connect_reply>(c, gc_connect_reply, &r);
}

static void forward(conn* src, connector *cr, msg_head *h, unsigned char *msg, size_t sz)
{
    uint64_t uid = 0;

    if (h->flags & FLAG_HAS_UID) {
        merror("forward connection %s's cmd:%d failed, uid exist!", src->addrtext, h->cmd);
        disconnect(src);
        return;
    }

    conn *dest = NULL;
    pthread_mutex_lock(&cr->lock);
    dest = cr->c;
    conn_incref(dest);
    pthread_mutex_unlock(&cr->lock);

    if (NULL == dest) {
        merror("forward connection %s's cmd:%d failed, dest not exist!", src->addrtext, h->cmd);
        disconnect(src);
        return;
    }

    conn_lock(dest);
    if (STATE_NOT_CONNECTED == dest->state) {
        merror("forward connection %s's cmd:%d failed, state == STATE_NOT_CONNECTED!", src->addrtext, h->cmd);
        disconnect(src);
        conn_unlock(dest);
    } else {
        evbuffer *output = bufferevent_get_output(dest->bev);
        if (!output) {
            merror("forward connection %s's cmd:%d failed, bufferevent_get_output!", src->addrtext, h->cmd);
            disconnect(src);
            conn_unlock(dest);
            return;
        }

        struct evbuffer_iovec v[1];

        evbuffer_lock(output);
        if (0 >= evbuffer_reserve_space(output, sz + sizeof(uint64_t), v, 1)) {
            merror("forward connection %s's cmd:%d failed, evbuffer_reserve_space!", src->addrtext, h->cmd);
            disconnect(src);
            evbuffer_unlock(output);
            conn_unlock(dest);
            return;
        }

        unsigned char *nmsg = (unsigned char *)(v[0].iov_base);
        memcpy(nmsg, msg, MSG_HEAD_SIZE);
        nmsg += sizeof(unsigned short);
        *(unsigned short *)nmsg = htons((unsigned short)(h->len + sizeof(uint64_t)));
        nmsg += (MSG_HEAD_SIZE - sizeof(unsigned short));
        *(uint64_t *)nmsg = htons(uid);
        nmsg += sizeof(uint64_t);
        memcpy(nmsg, msg + MSG_HEAD_SIZE, h->len);
        v[0].iov_len = MSG_HEAD_SIZE + h->len + sizeof(uint64_t);

        if (0 > evbuffer_commit_space(output, v, 1)) {
            merror("forward connection %s's cmd:%d failed, evbuffer_add!", src->addrtext, h->cmd);
            disconnect(src);
            evbuffer_unlock(output);
            conn_unlock(dest);
            return;
        }
        bufferevent_enable(dest->bev, EV_WRITE);
        evbuffer_unlock(output);
        conn_unlock(dest);
    }

    mdebug("forward cmd:%d for connection %s", h->cmd, src->addrtext);
}

void client_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        /* close connection */
        return;
    }

    if (h.cmd >= CS_BEGIN && h.cmd < CS_END) {
        /* client -> gate */
        if (h.cmd >= CG_BEGIN && h.cmd < CG_END) {
            if (cbs[h.cmd - CG_BEGIN]) {
                (*cbs[h.cmd - CG_BEGIN])(c, msg, sz);
            }
        /* client -> center */
        } else if (h.cmd >= CE_BEGIN && h.cmd < CE_END) {
            forward(c, center, &h, msg, sz);
        /* client -> cache */
        } else if (h.cmd >= CA_BEGIN && h.cmd < CA_END) {
            forward(c, cache, &h, msg, sz);
        /* client -> game */
        } else if (h.cmd >= CM_BEGIN && h.cmd < CM_END) {
            forward(c, cache, &h, msg, sz);
        } else {
            merror("invalid cmd:%d connection %s", h.cmd, c->addrtext);
            /* close connection */
        }
    } else {
        merror("invalid cmd:%d connection %s", h.cmd, c->addrtext);
        /* close connection */
    }
}

void client_connect_cb(conn *c, int ok)
{
    mdebug("client_connect_cb");
    /*
    pthread_rwlock_rdlock(&user_mgr->rwlock);
    user_map_t::iterator itr= user_mgr->users_.find(uid);
    if (itr != user_mgr->users_.end()) {
        user_t *user = itr->second;
        pthread_mutex_lock(&user->lock);
        if (user->state == 
        pthread_mutex_unlock(&user->lock);
    }
    pthread_rwlock_unlock(&user_mgr->rwlock);
    */
}

void client_disconnect_cb(conn *c)
{
    mdebug("client_disconnect_cb");
}

void client_cb_init(user_callback *cb)
{
    cb->rpc = client_rpc_cb;
    cb->connect = client_connect_cb;
    cb->disconnect = client_disconnect_cb;
    memset(cbs, 0, sizeof(cb) * (CG_END - CG_BEGIN));
    cbs[cg_connect_request - CG_BEGIN] = connect_request_cb;
}
