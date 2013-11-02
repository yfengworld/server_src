#ifndef FWD_H_INCLUDED
#define FWD_H_INCLUDED

#include "user.h"

#include "net.h"

#include <string>

extern std::string gate_ip;
extern short gate_port;

extern connector *center;
extern connector *game;
extern connector *cache;

extern user_manager_t *user_mgr;

#endif /* FWD_H_INCLUDED */
