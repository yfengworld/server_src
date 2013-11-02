#ifndef USER_H_INCLUDED
#define USER_H_INCLUDED

#include "net.h"

#include <map>

#include <pthread.h>
#include <inttypes.h>

typedef struct user_t user_t;
struct user_t {
    uint64_t uid;
    char sk[32];
    conn *c;
    int refcnt;
    pthread_mutex_t lock;
};

void user_lock(user_t *user);
void user_unlock(user_t *user);
void user_lock_incref(user_t *user);
int user_decref_unlock(user_t *user);
void user_incref(user_t *user);
int user_decref(user_t *user);

typedef std::map<uint64_t, user_t*> user_map_t;

class user_manager_t {
public:
    user_manager_t();

public:
    user_t* get_user_incref(uint64_t uid);
    int add_user(uint64_t uid);
    int del_user(uint64_t uid);

public:
    user_map_t users;
    pthread_rwlock_t rwlock;
};

#endif /* USER_H_INCLUDED */
