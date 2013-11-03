#ifndef GATE_INFO_H_INCLUDED
#define GATE_INFO_H_INCLUDED

#include "net.h"

#include <vector>

#include <pthread.h>

struct gate_info {
    conn *c;
    char ip[32];
    short port;
    int refcnt;
    pthread_mutex_t lock;
};

void gate_info_incref(struct gate_info *info);
void gate_info_decref(struct gate_info *info);

typedef std::vector<struct gate_info *> gate_info_vector_t;

class gate_info_manager_t {
public:
    gate_info_manager_t();

public:
    struct gate_info *get_best_gate_incref(uint64_t uid);
    int get_gate_ip_port(conn *c, char **ip, short *port);
    int add_gate_info(conn *c, const char *ip, short port);
    int del_gate_info(conn *c);

private:
    gate_info_vector_t gate_infos;
    pthread_rwlock_t rwlock;
};

#endif /* GATE_INFO_H_INCLUDED */
