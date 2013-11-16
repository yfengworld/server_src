#include "task_pool.h"
#include "log.h"

struct task {
	void *(*func)(void *);
	void *arg;
	struct task *prev;
	struct task *next;
};

static void *thread_func(void *arg)
{
	struct task_pool *pool = (struct task_pool *)arg;
	struct task* t;

	while (1) {
		pthread_mutex_lock(&pool->lock);
		while (NULL == pool->head && 0 == pool->shutdown) {
			pthread_cond_wait(&pool->cond, &pool->head);
		}
		if (pool->shutdown) {
			pthread_mutex_unlock(&pool->lock);
			break;
		}
		t = pool->head;
		pool->head = pool->head->next;
		if (NULL != pool->head->next)
			pool->head->next->prev = NULL;
		pthread_mutex_unlock(&pool->lock);

		if (NULL != t->func)
			t->func(t->arg);
		free(t);
	}
	return NULL;
}

struct task_pool *task_pool_new(int nthreads)
{
	int i;
	struct task_pool *pool = (struct task_pool *)malloc(struct task_pool);
	if (NULL == pool) {
		mfatal("task_pool alloc failed!");
		exit(1);
	}

	pool->shutdown = 0;
	pool->nthreads = nthreads;
	pool->thread = (pthread_t)malloc(nthreads * sizeof(pthread_t));
	if (NULL == pool->thread) {
		mfatal("thread id alloc failed!");
		exit(1);
	}
	for (i = 0; i < nthreads; i++) {
		if (0 != pthread_create(&pool->thread[i], NULL, thread_func, NULL)) {
			mfatal("pthread_create failed!");
			exit(1);
		}
	}
	pool->head = NULL;
	if (0 != pthread_mutex_init(&pool->lock, NULL)) {
		mfatal("pthread_mutex_init failed!");
		exit(1);
	}
	if (0 != pthread_cond_init(&pool->cond, NULL)) {
		mfatal("pthread_cond_init failed!");
		exit(1);
	}
	return pool;
}

void task_pool_free(struct task_pool *pool)
{
	int i;
	for (i = 0; i < pool->nthreads; i++) {
		pthread_join(pool->thread[i]);
	}
	free(pool->thread);
	free(pool);
}

int task_pool_add(struct task_pool *pool, void *(*func)(void *), void *arg)
{
	struct task *t, h;

	t = (struct task *)malloc(sizeof(struct task));
	if (NULL == t) {
		merror("task alloc failed!");
		return -1;
	}

	t->func = func;
	t->arg = arg;
	t->prev = NULL;
	t->next = NULL;

	pthread_mutex_lock(&pool->lock);
	if (NULL == pool->head) {
		pool->head = t;
		pool->tail = t;
	} else {
		t->prev = pool->tail;
		pool->tail->next = t;
		pool->tail = t;
	}
	pthread_cond_signal(&pool->cond);
	pthread_mutex_unlock(&pool->lock);

	return 0;
}
