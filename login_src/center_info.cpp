#include "center_info.h"

#include "log.h"

#include <stdlib.h>

void center_info_lock(center_info_t *info)
{
    pthread_mutex_lock(&info->lock);
}

void center_info_unlock(center_info_t *info)
{
    pthread_mutex_unlock(&info->lock);
}

void center_info_lock_incref(center_info_t *info)
{
    pthread_mutex_lock(&info->lock);
    ++info->refcnt;
}

int center_info_decref_unlock(center_info_t *info)
{
    if (--info->refcnt) {
        pthread_mutex_unlock(&info->lock);
        return 0;
    }

    if (info->c) {
        conn_decref(info->c);
    }
    
    free(info);
    pthread_mutex_unlock(&info->lock);

    return 1;
}

void center_info_incref(center_info_t *info)
{
    pthread_mutex_lock(&info->lock);
    ++info->refcnt;
    pthread_mutex_unlock(&info->lock);
}

int center_info_decref(center_info_t *info)
{
    pthread_mutex_lock(&info->lock);
    return center_info_decref_unlock(info);
}

center_info_manager_t::center_info_manager_t()
{
    pthread_rwlock_init(&rwlock, NULL);
}

center_info_t *center_info_manager_t::get_center_info_incref(int id)
{
    center_info_t *info = NULL;

    pthread_rwlock_rdlock(&rwlock);
    center_info_map_t::iterator itr = center_infos.find(id);
    if (itr != center_infos.end()) {
        info = itr->second;
        center_info_incref(info);
    }
    pthread_rwlock_unlock(&rwlock);

    return info;
}

int center_info_manager_t::add_center_info(int id, conn *c)
{
    int ret = -1;

    pthread_rwlock_wrlock(&rwlock);
    center_info_map_t::iterator itr = center_infos.find(id);
    if (itr != center_infos.end()) {
        center_info_t *info = itr->second;
        center_info_lock(info);
        if (info->c) {
            conn_decref(info->c);
            info->c = NULL;
        }
        info->c = c;
        conn_incref(info->c);
        center_info_unlock(info);
    } else {
        center_info_t *info = (center_info_t *)malloc(sizeof(center_info_t));
        if (NULL == info) {
            merror("center_info_t alloc failed!");
            ret = -1;
        }
        info->id = id;
        info->c = c;
        conn_lock_incref(info->c);
        info->c->arg = (void *)id;
        conn_unlock(info->c);
        info->refcnt = 1;
        pthread_mutex_init(&info->lock, NULL);
        center_info_incref(info);
        center_infos.insert(std::make_pair(id, info));
    }
    pthread_rwlock_unlock(&rwlock);

    return ret;
}

int center_info_manager_t::del_center_info(int id)
{
    int ret = -1;

    pthread_rwlock_wrlock(&rwlock);
    center_info_map_t::iterator itr = center_infos.find(id);
    if (itr != center_infos.end()) {
        center_info_t *info = itr->second;
        center_info_decref(info);
        center_infos.erase(itr);
        ret = 0;
    }
    pthread_rwlock_unlock(&rwlock);

    return ret;
}
