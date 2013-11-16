#ifndef DB_TASK_H_INCLUDED
#define DB_TASK_H_INCLUDED

#include <pthread.h>

/* forward declare */
struct task_pool;
struct db_task_queue;

#define DB_TASK_SUCCESS 0
#define DB_TASK_WAIT 99

struct db_task {
	int result;
	void *arg;
	void (*free)();
	void (*exec)();
	struct db_task_queue *queue;
	void (*logic_exec)();
};

struct db_task_queue {
	void *ctx;
	int num;
	struct db_task *head;
	struct db_task *tail;
	pthread_mutex_t lock;
};

struct db_task_queue *db_task_queue_new(void *ctx);
void db_task_queue_free(struct db_task_queue *queue);
int post_db_task(struct task_pool *pool, struct db_task_queue *queue,
				void *arg, void (*free)(), 
				void (*exec)(),
				void (*logic_exec)());
void db_task_process(struct db_task_queue *queue);

#endif /* DB_TASK_H_INCLUDED */
