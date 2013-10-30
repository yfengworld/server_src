#include "log.h"
#include "net.h"
#include "cmd.h"
#include "fwd.h"
#include "logic_thread.h"

#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define WORKER_NUM 1

LOGIC_THREAD logic;

static void signal_cb(evutil_socket_t, short, void *);

/* server_cb */
static user_callback server_cb;
static user_callback logic_server_cb;
void server_cb_init(user_callback *, user_callback *);

int main(int argc, char **argv)
{
    /* open log */
    if (0 != LOG_OPEN("./client", LOG_LEVEL_DEBUG, -1)) {
        fprintf(stderr, "open center log failed!\n");
        return 1;
    }

    if (0 != check_cmd()) {
        return 1;
    }
    server_cb_init(&server_cb, &logic_server_cb);

    /* protobuf verify version */
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    struct event_base *main_base = event_base_new();
    if (NULL == main_base) {
        mfatal("main_base = event_base_new() failed!");
        return 1;
    }

    conn_init();

    /* thread */
    pthread_t worker[WORKER_NUM];
    thread_init(main_base, WORKER_NUM, worker);

    /* logic thread */
    logic_thread_init(&logic, &logic_server_cb);

    /* signal */
    struct event *signal_event;
    signal_event = evsignal_new(main_base, SIGINT, signal_cb, (void *)main_base);
    if (NULL == signal_event || 0 != event_add(signal_event, NULL)) {
        mfatal("create/add a signal event failed!");
        return 1;
    }

    /* connector to login */
    struct sockaddr_in csa;
    bzero(&csa, sizeof(csa));
    csa.sin_family = AF_INET;
    csa.sin_addr.s_addr = inet_addr("127.0.0.1");
    csa.sin_port = htons(41000);

    connector *cl = connector_new((struct sockaddr *)&csa, sizeof(csa),
            server_cb.rpc,
            server_cb.connect,
            server_cb.disconnect);
    if (NULL == cl) {
        mfatal("create login connector failed!");
        return 1;
    }

    event_base_dispatch(main_base);

    pthread_join(logic.thread, NULL);

    for (int i = 0; i < WORKER_NUM; i++)
        pthread_join(worker[i], NULL);

    connector_free(cl);
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
