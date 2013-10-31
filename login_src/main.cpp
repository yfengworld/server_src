#include "fwd.h"
#include "user.h"

#include "msg_protobuf.h"
#include "cmd.h"
#include "net.h"
#include "log.h"

#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define WORKER_NUM 8

static void signal_cb(evutil_socket_t, short, void *);

/* client_cb */
static user_callback client_cb;
void client_cb_init(user_callback *cb);

/* center_cb */
static user_callback center_cb;
void center_cb_init(user_callback *cb);

user_manager_t *user_mgr = NULL;

int main(int argc, char **argv)
{
    /* open log */
    if (0 != LOG_OPEN("./login", LOG_LEVEL_DEBUG, -1)) {
        return 1;
    }

    /* cmd */
    if (0 != check_cmd()) {
        return 1;
    }
    client_cb_init(&client_cb);
    center_cb_init(&center_cb);

    /* protobuf verify version */
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    struct event_base *main_base = event_base_new();
    if (NULL == main_base) {
        mfatal("main_base = event_base_new() failed!");
        return 1;
    }

    conn_init();

    /* worker thread */
    pthread_t worker[WORKER_NUM];
    thread_init(main_base, WORKER_NUM, worker);

    /* signal */
    struct event *signal_event;
    signal_event = evsignal_new(main_base, SIGINT, signal_cb, (void *)main_base);
    if (NULL == signal_event || 0 != event_add(signal_event, NULL)) {
        mfatal("create/add a signal event failed!");
        return 1;
    }

    /* listener for client */
    struct sockaddr_in sa;
    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(41000);

    listener *lc = listener_new(main_base, (struct sockaddr *)&sa, sizeof(sa),
            client_cb.rpc,
            client_cb.connect,
            client_cb.disconnect);
    if (NULL == lc) {
        mfatal("create client listener failed!");
        return 1;
    }

    /* listener for center */
    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(41001);

    listener *le = listener_new(main_base, (struct sockaddr *)&sa, sizeof(sa),
            center_cb.rpc,
            center_cb.connect,
            center_cb.disconnect);
    if (NULL == le) {
        mfatal("create center listener failed!");
        return 1;
    }
    
    user_mgr = user_manager_new();
    if (NULL == user_mgr) {
        mfatal("create user manager failed!");
        return 1;
    }

    event_base_dispatch(main_base);

    for (int i = 0; i < WORKER_NUM; i++)
        pthread_join(worker[i], NULL);

    listener_free(lc);
    listener_free(le);
    event_free(signal_event);
    event_base_free(main_base);

    /* shutdown protobuf */
    google::protobuf::ShutdownProtobufLibrary();

    /* close log */
    LOG_CLOSE();

    return 0;
}

void signal_cb(evutil_socket_t fd, short what, void *arg)
{
    mdebug("signal_cb");
    struct event_base *base = (struct event_base *)arg;
    event_base_loopbreak(base);
    dispatch_conn_new(-1, 'k', NULL);
}
