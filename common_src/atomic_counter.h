#ifndef ATOMIC_COUNTER_H_INCLUDED
#define ATOMIC_COUNTER_H_INCLUDED

struct atomic_counter {
	volatile long count;
};

void atomic_counter_init(struct atomic_counter *c);
long atomic_counter_inc(struct atomic_counter *c);
long atomic_counter_dec(struct atomic_counter *c);

#endif /* ATOMIC_COUNTER_H_INCLUDED */
