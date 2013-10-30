#include "net.h"
#include "cmd.h"
#include "fwd.h"

typedef void (*cb)(conn *, unsigned char *msg, size_t sz);
static cb cbs[CG_END - CG_BEGIN];

static void user_session_cb(conn *c, unsigned char *msg, size_t sz)
{
    mdebug("user_session_cb");
}

static void forward(conn* c, connector *cr, msg_head *h, unsigned char *msg, size_t sz)
{
    uint64_t uid = 0;

    if (h->flags & FLAG_HAS_UID) {
        merror("should no uid connection %s", c->addrtext);
        /* close connection */
        disconnect(c);
        return;
    }

    if (cr->state != STATE_CONNECTED || !cr->c || !cr->c->bev) {
        merror("forward connection %s's cmd:%d failed!", c->addrtext, h->cmd);
        return;
    }

    evbuffer *output = bufferevent_get_output(cr->c->bev);
    if (!output) {
        merror("forward connection %s's cmd:%d failed, bufferevent_get_output!", c->addrtext, h->cmd);
        return;
    }

    struct evbuffer_iovec v[1];

    evbuffer_lock(output);
    if (0 >= evbuffer_reserve_space(output, sz + sizeof(uint64_t), v, 1)) {
        evbuffer_unlock(output);
        merror("forward connection %s's cmd:%d failed, evbuffer_reserve_space!", c->addrtext, h->cmd);
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
        evbuffer_unlock(output);
        merror("forward connection %s's cmd:%d failed, evbuffer_add!", c->addrtext, h->cmd);
        return;
    }
    bufferevent_enable(cr->c->bev, EV_WRITE);
    evbuffer_unlock(output);

    disconnect(c);

    mdebug("forward cmd:%d for connection %s", h->cmd, c->addrtext);
}

void client_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        /* close connection */
        disconnect(c);
        return;
    }

    if (h.cmd >= CG_BEGIN && h.cmd < CS_END) {
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
            disconnect(c);
        }
    } else {
        merror("invalid cmd:%d connection %s", h.cmd, c->addrtext);
        /* close connection */
        disconnect(c);
    }
}

void client_connect_cb(conn *c)
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
    cbs[cg_user_session - CG_BEGIN] = user_session_cb;
}
