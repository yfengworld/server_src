#include "gate_info.h"

#include "log.h"

#include <stdlib.h>

void gate_info_incref(struct gate_info *info)
{
    pthread_mutex_lock(&info->lock);
    ++info->refcnt;
    pthread_mutex_unlock(&info->lock);
}

void gate_info_decref(struct gate_info *info)
{
    pthread_mutex_lock(&info->lock);
    if (--info->refcnt)
        return;
    
    conn_decref(info->c);
    free(info);

    pthread_mutex_unlock(&info->lock);
}


gate_info_manager_t::gate_info_manager_t()
{
    pthread_mutex_init(&lock, NULL);
}

struct gate_info *gate_info_manager_t::get_best_gate_incref(uint64_t uid)
{
    struct gate_info *info = NULL;

    pthread_mutex_lock(&lock);
    size_t sz = gate_infos.size();
    if (sz > 0) {
        int idx = (int)(uid % (uint64_t)sz);
        info = gate_infos[idx];
        gate_info_incref(info);
    }
    pthread_mutex_unlock(&lock);

    return info;
}

int gate_info_manager_t::add_gate_info(conn *c, const char *ip, short port)
{
    gate_info *info = (gate_info *)malloc(sizeof(gate_info));
    if (NULL == info) {
        merror("gate_info alloc failed!");
        return -1;
    }

    info->c = c;
    conn_incref(c);

    info->ip = ip;
    info->port = port;
    info->refcnt = 1;
    pthread_mutex_init(&info->lock, NULL);
    gate_info_incref(info);

    pthread_mutex_lock(&lock);
    gate_infos.push_back(info);
    pthread_mutex_unlock(&lock);

    return 0;
}

int gate_info_manager_t::del_gate_info(conn *c)
{
    int ret = -1;
    pthread_mutex_lock(&lock);
    gate_info_vector_t::iterator itr = gate_infos.begin();
    for (; itr != gate_infos.end(); ++itr) {
        if ((*itr)->c == c) {
            conn_decref(c);
            delete *itr;
            gate_infos.erase(itr);
            ret = 0;
            break;
        }
    }
    pthread_mutex_unlock(&lock);
    return ret;
}
