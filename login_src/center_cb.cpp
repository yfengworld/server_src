#include "net.h"
#include "log.h"
#include "cmd.h"
#include "msg_protobuf.h"

#include "fwd.h"

center_conn *centers = NULL;
pthread_rwlock_t centers_rwlock = PTHREAD_RWLOCK_INITIALIZER;

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[EL_END - EL_BEGIN];

static void user_login_reply_cb(conn *c, unsigned char *msg, size_t sz)
{
    mdebug("user_login_reply_cb");
}

static void center_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        merror("message_head failed!");
        return;
    }

    if (h.cmd > EL_BEGIN && h.cmd < EL_END) {
        if (cbs[h.cmd - EL_BEGIN]) {
            if (cbs[h.cmd - EL_BEGIN]) {
                (*(cbs[h.cmd - EL_BEGIN]))(c, msg, sz);
            }
        } else {
            merror("invalid cmd:%d", h.cmd);
            return;
        }
    }
}

static void center_connect_cb(conn *c, int ok)
{
    mdebug("center_connect_cb");
    center_conn *cc = (center_conn *)malloc(sizeof(center_conn));
    if (NULL == cc) {
        disconnect(c);
        return;
    }

    pthread_rwlock_wrlock(&centers_rwlock);
    cc->c = c;
    cc->next = centers;
    centers = cc;
    pthread_rwlock_unlock(&centers_rwlock);
}

static void center_disconnect_cb(conn *c)
{
    mdebug("center_disconnect_cb");
    pthread_rwlock_wrlock(&centers_rwlock);
    center_conn *cc = centers;
    center_conn *prev = NULL;
    while (cc) {
        if (cc->c == c) {
            if (prev)
                prev->next = cc->next;
            disconnect(c);
            free(cc);
            break;
        }
        prev = cc;
        cc = cc->next;
    }
    pthread_rwlock_unlock(&centers_rwlock);
}

void center_cb_init(user_callback *cb)
{
    cb->rpc = center_rpc_cb;
    cb->connect = center_connect_cb;
    cb->disconnect = center_disconnect_cb;

    memset(cbs, 0, sizeof(cb) * (EL_END - EL_BEGIN));
    cbs[el_user_login_reply - EL_BEGIN] = user_login_reply_cb;
}
