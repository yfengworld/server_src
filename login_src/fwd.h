#ifndef FWD_H_INCLUDED
#define FWD_H_INCLUDED

#include <zdb.h>

/* forward declare */
class user_manager_t;
class center_info_manager_t;

extern ConnectionPool_T pool;
extern user_manager_t *user_mgr;
extern center_info_manager_t *center_info_mgr;

#endif /* FWD_H_INCLUDED */
