#include "fwd.h"
#include "user.h"
#include "center_info.h"
#include "login.pb.h"

#include "msg_protobuf.h"
#include "cmd.h"
#include "net.h"
#include "log.h"

typedef void (*cb)(conn *, unsigned char *, size_t);
static cb cbs[CL_END - CL_BEGIN];

struct timeout_info {
    struct event *timer;
    int tempid;
    conn *c;
};

static void expire_timer_cb(int fd, short what, void *arg)
{
    return;
    mdebug("expire_timer_cb");
    struct timeout_info *ti = (struct timeout_info *)arg;
    disconnect(ti->c);
    free(ti->timer);
    free(ti);
}

static void login_request_cb(conn *c, unsigned char *msg, size_t sz)
{
	Connection_T dbc = ConnectionPool_getConnection(pool);
	if (NULL == dbc) {
		mwarn("ConnectionPool_getConnection failed!");
		return;
	}

	TRY {
		/* check account && passwd */
		login::login_request lr;
		msg_body<login::login_request>(msg, sz, &lr);
		ResultSet_T result = Connection_executeQuery(dbc,
				"SELECT `uid` FROM `profile` WHERE `account`=%s AND `passwd`=%s;",
				lr.account().c_str(), lr.passwd().c_str());

		if (ResultSet_next(result)) {
			uint64_t uid = ResultSet_getLLong(result, 1);
			login::error err = login::success;
			do {
				int tempid = user_manager_t::get_guid();
				user_t *user = (user_t *)malloc(sizeof(user_t));
				if (NULL == user) {
					mdebug("user_t alloc failed!");
					err = login::unknow;
					break;
				}
				user->id = tempid;
				user->c = NULL;
				user->refcnt = 1;
				pthread_mutex_init(&user->lock, NULL);

				if (0 > user_mgr->add_user(user)) {
					mdebug("add_user failed!");
					err = login::unknow;
					break;
				}

				user_lock(user);
				user->c = c;
				user_unlock(user);

				conn_lock(c);
				c->user = user;
				conn_unlock(c);

				/* tell center */
				login::user_login_request ulr;
				ulr.set_tempid(tempid);
				ulr.set_uid(uid);
				center_info_t *info = center_info_mgr->get_center_info_incref(1);
				if (info) {
					conn_write<login::user_login_request>(info->c, le_user_login_request, &ulr);
					center_info_decref(info);
				}
				return;
			} while (0);
			login::login_reply lr;
			lr.set_err(err);
			conn_write<login::login_reply>(c, lc_login_reply, &lr);
		} else {
			login::login_reply lr;
			lr.set_err(login::auth);
			conn_write<login::login_reply>(c, lc_login_reply, &lr);
		}
	} CATCH(SQLException) {
		merror("SQLException -- %s", Exception_frame.message);
	} FINALLY {
		Connection_close(dbc);
	} END_TRY;
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
	mdebug("client -> login cmd:%d len:%d flags:%d", h.cmd, h.len, h.flags);

	if (h.cmd > CL_BEGIN && h.cmd < CL_END) {
		if (cbs[h.cmd - CL_BEGIN]) {
			(*(cbs[h.cmd - CL_BEGIN]))(c, msg, sz);
		}
	} else {
		merror("client -> login invalid cmd:%d len:%d flags:%d", h.cmd, h.len, h.flags);
		/* close connection */
		disconnect(c);
		return;
	}
}

void client_connect_cb(conn *c, int ok)
{
	mdebug("client_connect_cb ok:%d", ok);
	if (0 == ok)
		return;
	do
	{
		struct timeout_info *ti = (struct timeout_info *)malloc(sizeof(struct timeout_info));
		if (NULL == ti)
			break;

		ti->c = c;

		ti->timer = evtimer_new(c->thread->base, expire_timer_cb, ti);
		if (NULL == ti->timer) {
			free(ti);
			break;
		}
		struct timeval tv = {5, 0};
		if (0 > evtimer_add(ti->timer, &tv)) {
			free(ti->timer);
			free(ti);
			break;
		}
		return;

	} while (0);

	disconnect(c);
}

void client_disconnect_cb(conn *c)
{
	mdebug("client_disconnect_cb");
	if (c->user) {
		user_mgr->del_user((user_t *)c->user);
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
