#include "db_task.h"

struct db_task_queue *db_task_queue_new(void *ctx)
{
	struct db_task_queue *queue = (struct db_task_queue *)malloc(sizeof(struct db_task_queue));
	if (NULL == queue) {
		mfatal("db_task_queue alloc failed!");
		exit(1);
	}
	queue->ctx = ctx;
	queue->num = 0;
	queue->prev = NULL;
	queue->next = NULL;
	if (0 != pthread_mutex_init(&queue->lock, NULL)) {
		mfatal("pthread_mutex_init failed!");
		exit(1);
	}
	return queue;
}

void db_task_queue_free(struct db_task_queue *queue)
{
	/* the queue must be empty */
	free(queue);
}

static void db_task_queue_push(struct db_task_queue *queue, struct db_task *t)
{
	pthread_mutex_lock(&queue->lock);
	if (0 == queue->num) {
		t->prev = NULL;
		t->next = NULL;
		queue->head = t;
		queue->tail = t;
	} else {
		queue->tail->next = t;
		t->prev = queue->tail;
		t->next = NULL;
		queue->tail = t;
	}
	queue->num++;
	pthread_mutex_unlock(&queue->lock);
}

static struct db_task *db_task_queue_pop(struct db_task_queue *queue)
{
	struct db_task *t = NULL;
	pthread_mutex_lock(&queue->lock);
	if (0 < queue->num) {
		t = queue->head;
		if (queue->head->next)
			queue->head->next->prev = NULL;
		else
			queue->tail = queue->head->next;
		queue->head = queue->head->next;
		queue->num--;
	}
	pthread_mutex_unlock(&queue->lock);
	return t;
}

static void *exec_db_task(void *arg)
{
	struct db_task *t = (struct db_task *)arg;
	if (t->exec)
		t->exec();
	if (t->queue)
		db_task_queue_push(t->queue, t);
}

int post_db_task(struct task_pool *pool, struct db_task_queue *queue,
				void *arg, void (*free)(), 
				void (*exec)(),
				void (*logic_exec)())
{
	struct db_task *t = (struct db_task *)malloc(sizeof(struct db_task));
	if (NULL == t) {
		merror("db_task alloc failed!");
		return -1;
	}
	t->result = DB_TASK_WAIT;
	t->arg = arg;
	t->free = free;
	t->exec = exec;
	t->queue = queue;
	t->logic_exec = logic_exec;
	if (0 != task_pool_add(pool, exec_db_task, t)) {
		merror("post db_task failed!");
		return -1;
	}
	return 0;
}

void db_task_process(struct db_task_queue *queue)
{
	struct db_task *t;
	while (1) {
		t = db_task_queue_pop(queue);
		if (t && t->logic_exec)
			t->logic_exec();
		break;
	}
}
