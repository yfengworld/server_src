#include "user.h"

#include "log.h"

#include <stdio.h>
#include <stdlib.h>

void user_lock(user_t *user)
{
    pthread_mutex_lock(&user->lock);
}

void user_unlock(user_t *user)
{
    pthread_mutex_unlock(&user->lock);
}

void user_lockincref(user_t *user)
{
    pthread_mutex_lock(&user->lock);
    ++user->refcnt;
}

int user_decref_unlock(user_t *user)
{
    if (--user->refcnt) {
        pthread_mutex_unlock(&user->lock);
        return -1;
    }

    if (user->c)
        conn_decref(user->c);

    free(user);
    pthread_mutex_unlock(&user->lock);

    return 0;
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

user_manager_t::user_manager_t()
{
    pthread_rwlock_init(&rwlock, NULL);
}

user_t* user_manager_t::get_user_incref(uint64_t uid)
{
    user_t* user = NULL;

    pthread_rwlock_rdlock(&rwlock);
    user_map_t::iterator itr = users.find(uid);
    if (itr != users.end()) {
        user = itr->second;
        user_incref(user);
    }
    pthread_rwlock_unlock(&rwlock);

    return user;
}

int user_manager_t::add_user(uint64_t uid)
{
    int ret = -1;

    pthread_rwlock_wrlock(&rwlock);
    std::pair<user_map_t::iterator, bool> ib = users.insert(std::make_pair(uid, (user_t*)NULL));
    //user_map_t::iterator itr = users.find(uid);
    if (ib.second) {
        user_t *user = (user_t *)malloc(sizeof(user_t));
        if (NULL == user) {
            merror("new user_t failed!");
        } else {
            user->uid = uid;
            user->c = NULL;
            user->refcnt = 1;
            pthread_mutex_init(&user->lock, NULL);
            //users.insert(std::make_pair(uid, user));
            ib.first->second = user;
            ret = 0;
        }
    }
    pthread_rwlock_unlock(&rwlock);

    return ret;
}

int user_manager_t::del_user(uint64_t uid)
{
    int ret = -1;

    pthread_rwlock_wrlock(&rwlock);
    user_map_t::iterator itr = users.find(uid);
    if (itr != users.end()) {
        user_decref(itr->second);
        users.erase(itr);
        ret = 0;
    }
    pthread_rwlock_unlock(&rwlock);

    return ret;
}
