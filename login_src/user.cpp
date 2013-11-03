#include "user.h"

#include "log.h"

#include <stdlib.h>

void user_lock(user_t *user)
{
    pthread_mutex_lock(&user->lock);
}

void user_unlock(user_t *user)
{
    pthread_mutex_unlock(&user->lock);
}

void user_lock_incref(user_t *user)
{
    pthread_mutex_lock(&user->lock);
    ++user->refcnt;
}

int user_decref_unlock(user_t *user)
{
    if (--user->refcnt) {
        pthread_mutex_unlock(&user->lock);
        return 0;
    }

    if (user->c) {
        conn_lock_incref(user->c);
        user->c->user = NULL;
        conn_decref_unlock(user->c);
        user->c = NULL;
    }

    free(user);
    mdebug("user tempid:%d free!", user->id);
    return 1;
}

void user_incref(user_t *user)
{
    pthread_mutex_lock(&user->lock);
    ++user->refcnt;
    pthread_mutex_unlock(&user->lock);
}

int user_decref(user_t *user)
{
    pthread_mutex_lock(&user->lock);
    return user_decref_unlock(user);
}

int user_manager_t::guid = 0;

user_manager_t::user_manager_t()
{
    pthread_rwlock_init(&rwlock, NULL);
}

int user_manager_t::get_guid()
{
    return ++guid;
}

user_t *user_manager_t::get_user_incref(int id)
{
    user_t *user = NULL;

    pthread_rwlock_rdlock(&rwlock);
    user_map_t::iterator itr = users.find(id);
    if (itr != users.end()) {
        user = itr->second;
        user_incref(user);
    }
    pthread_rwlock_unlock(&rwlock);

    return user;
}

int user_manager_t::add_user(user_t *user)
{
    int ret = -1;

    pthread_rwlock_wrlock(&rwlock);
    user_map_t::iterator itr = users.find(user->id);
    if (itr == users.end()) {
        user_incref(user);
        users.insert(std::make_pair(user->id, user));
        ret = 0;
    }
    pthread_rwlock_unlock(&rwlock);

    return ret;
}

int user_manager_t::del_user(user_t *user)
{
    int ret = -1;

    pthread_rwlock_rdlock(&rwlock);
    user_map_t::iterator itr = users.find(user->id);
    if (itr != users.end()) {
        user_decref(itr->second);
        users.erase(itr);
        ret = 0;
    }
    pthread_rwlock_unlock(&rwlock);

    return ret;
}
