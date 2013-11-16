#ifndef TASK_POOL_H_INCLUDED
#define TASK_POOL_H_INCLUDED

struct task_pool {
	int shutdown;
	int nthreads;
	pthread_t *thread;
	struct task *head;
	struct task *tail;
	pthread_mutex_t lock;
	pthread_cond_t cond;
};

struct task_pool *task_pool_new(int nthreads);
void task_pool_free(struct task_pool *pool);
int task_pool_add(struct task_pool *pool, void *(*func)(void *), void *arg);

#endif /* TASK_POOL_H_INCLUDED */
