#include "atomic_counter.h"
#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

void atomic_counter_init(struct atomic_counter *c)
{
	c->count = 0;
}

long atomic_counter_inc(struct atomic_counter *c)
{
#ifdef __GNUC__
	return __sync_fetch_and_add(&c->count, 1);
#else
#ifdef _WIN32
	return InterlockedIncrement(&c->count);
#endif /* _WIN32 */
#endif /* __GNUC__ */
}

long atomic_counter_dec(struct atomic_counter *c)
{
#ifdef __GNUC__
	return __sync_fetch_and_sub(&c->count, 1);
#else
#ifdef _WIN32
	return InterlockedDecrement(&c->count);
#endif /* _WIN32 */
#endif /* __GNUC__ */
}
