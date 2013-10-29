#include "net.h"
#include "fwd.h"

center_conn *centers = NULL;
pthread_rwlock_t centers_rwlock = PTHREAD_RWLOCK_INITIALIZER;


void center_rpc_cb(conn *c, unsigned char *msg, size_t sz)
{

}

void center_connect_cb(conn *c)
{
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

void center_disconnect_cb(conn *c)
{
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
