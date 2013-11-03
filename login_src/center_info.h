#ifndef CENTER_H_INCLUDED
#define CENTER_H_INCLUDED

#include "net.h"

#include <map>

#include <pthread.h>

typedef struct center_info_t center_info_t;
struct center_info_t {
    int id;
    conn *c;
    int refcnt;
    pthread_mutex_t lock;
};

void center_info_lock(center_info_t *info);
void center_info_unlock(center_info_t *info);
void center_info_lock_incref(center_info_t *info);
int center_info_decref_unlock(center_info_t *info);
void center_info_incref(center_info_t *info);
int center_info_decref(center_info_t *info);

typedef std::map<int, center_info_t *> center_info_map_t;

class center_info_manager_t {
public:
    center_info_manager_t();

public:
    center_info_t *get_center_info_incref(int id);
    int add_center_info(int id, conn *c);
    int del_center_info(int id);

public:
    center_info_map_t center_infos;
    pthread_rwlock_t rwlock;
};

#endif /* CENTER_H_INCLUDED */
