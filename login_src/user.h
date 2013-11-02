#ifndef USER_H_INCLUDED
#define USER_H_INCLUDED

#include "net.h"

#include <map>

#include <pthread.h>

#define MAX_SESSION_SECS 30

class user_t
{
public:
    user_t(int id);
    ~user_t();

public:
    int get_id();
    struct event *get_expire_timer();
    void set_conn(conn *c);
    void lock_incref();
    int decref_unlock();
    void incref();
    int decref();

public:
    static int get_guid();

private:
    static int guid;

private:
    int id_;
    conn *c_;
    struct event timer;
    int refcnt;
    pthread_mutex_t lock;
};

typedef std::map<int, user_t*> user_map_t;

class user_manager_t
{
public:
    user_manager_t();
    
public:
    user_t *get_user_incref(int id);
    int add_user(user_t *user);
    int del_user(user_t *user);

public:
    void rdlock();
    void wrlock();
    void unlock();

private:
    user_map_t users_;
    pthread_rwlock_t rwlock;
};

#endif /* USER_H_INCLUDED */
