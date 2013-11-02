#include "user.h"

#include "log.h"

int user_t::guid = 0;

user_t::user_t(int id)
    : id_(id)
    , refcnt(1)
{
    pthread_mutex_init(&lock, NULL);
}

user_t::~user_t()
{
    mdebug("user_t id=%d free", id_);
}

int user_t::get_id()
{
    return id_;
}

struct event *user_t::get_expire_timer()
{
    return &timer;
}

void user_t::set_conn(conn *c)
{
    lock_incref();
    c_ = c;
    decref_unlock();
    conn_incref(c);
    conn_lock_incref(c);
    c->user = this;
    conn_decref_unlock(c);
}

void user_t::lock_incref()
{
    pthread_mutex_lock(&lock);
    ++refcnt;
}

int user_t::decref_unlock()
{
    if (--refcnt) {
        pthread_mutex_unlock(&lock);
        return 0;
    }

    if (c_) {
        conn_lock_incref(c_);
        c_->user = NULL;
        conn_decref_unlock(c_);
        c_ = NULL;
    }

    delete this;
    return 1;
}

void user_t::incref()
{
    pthread_mutex_lock(&lock);
    ++refcnt;
    pthread_mutex_unlock(&lock);
}

int user_t::decref()
{
    pthread_mutex_lock(&lock);
    return decref_unlock();
}

int user_t::get_guid()
{
    return ++guid;
}

user_manager_t::user_manager_t()
{
    pthread_rwlock_init(&rwlock, NULL);
}

user_t *user_manager_t::get_user_incref(int id)
{
    user_t *user = NULL;
    rdlock();
    user_map_t::iterator itr = users_.find(id);
    if (itr != users_.end()) {
        user = itr->second;
        user->incref();
    }
    unlock();
    return user;
}

int user_manager_t::add_user(user_t *user)
{
    int ret = -1;
    rdlock();
    user_map_t::iterator itr = users_.find(user->get_id());
    if (itr == users_.end()) {
        users_.insert(std::make_pair(user->get_id(), user));
        ret = 0;
    }
    unlock();
    return ret;
}

int user_manager_t::del_user(user_t *user)
{
    int ret = -1;
    rdlock();
    user_map_t::iterator itr = users_.find(user->get_id());
    if (itr != users_.end()) {
        users_.erase(itr);
        ret = 0;
    }
    unlock();
    return ret;
}

void user_manager_t::rdlock()
{
    pthread_rwlock_rdlock(&rwlock);
}

void user_manager_t::wrlock()
{
    pthread_rwlock_wrlock(&rwlock);
}

void user_manager_t::unlock()
{
    pthread_rwlock_unlock(&rwlock);
}

