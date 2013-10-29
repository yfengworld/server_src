#include "net.h"
#include "cmd.h"
#include "fwd.h"
#include "login.pb.h"

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[CL_END - CL_BEGIN];

static void login_request_cb(conn *c, unsigned char *msg, size_t sz)
{
    mdebug("login_request_cb");
    /* account passwd check */

    pthread_rwlock_rdlock(&centers_rwlock);
    if (centers)
    {
        user_login_request ulr;
        ulr.set_uid(1);
        conn_write<user_login_request>(centers->c, le_user_login_request, &ulr);
        pthread_rwlock_unlock(&centers_rwlock);
        /* add user to map */
    } else {
        pthread_rwlock_unlock(&centers_rwlock);
    }
}

void client_cb_reg()
{
    memset(cbs, 0, sizeof(cb) * (CL_END - CL_BEGIN));
    cbs[cl_login_request - CL_BEGIN] = login_request_cb;
}

void client_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{
    msg_head h;
    if (0 != message_head(msg, sz, &h)) {
        merror("message_head failed!");
        /* close connection */
        return;
    }

    if (h.cmd > CL_BEGIN && h.cmd < CL_END) {
        if (cbs[h.cmd - CL_BEGIN]) {
            (*(cbs[h.cmd - CL_BEGIN]))(c, msg, sz);
        }
    } else {
        merror("invalid cmd:%d", h.cmd);
        /* close connection */
        return;
    }
}

void client_connect_cb(conn *c)
{
    mdebug("client_connect_cb");
}

void client_disconnect_cb(conn *c)
{
    mdebug("client_disconnect_cb");
}
