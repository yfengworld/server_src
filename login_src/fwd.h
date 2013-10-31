#ifndef FWD_H_INCLUDED
#define FWD_H_INCLUDED

#include "net.h"
#include "user.h"

#include <pthread.h>

extern user_manager_t *user_mgr;

typedef struct center_conn center_conn;
struct center_conn {
    conn *c;
    center_conn *next;
};

extern center_conn *centers;
extern pthread_rwlock_t centers_rwlock;

#endif /* FWD_H_INCLUDED */
