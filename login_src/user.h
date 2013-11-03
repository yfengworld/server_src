#ifndef USER_H_INCLUDED
#define USER_H_INCLUDED

#include "net.h"

#include <map>

#include <pthread.h>

#define MAX_SESSION_SECS 30

typedef struct user_t user_t;
struct user_t {
    int id;
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

typedef std::map<int, user_t*> user_map_t;

class user_manager_t
{
public:
    user_manager_t();
    
public:
    static int get_guid();

public:
    user_t *get_user_incref(int id);
    int add_user(user_t *user);
    int del_user(user_t *user);

public:
    static int guid;
    user_map_t users;
    pthread_rwlock_t rwlock;
};

#endif /* USER_H_INCLUDED */
