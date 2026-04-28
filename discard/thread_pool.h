#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <stdatomic.h>
#include <pthread.h>

#define MAX_THREAD_COUNT 4

typedef struct Task Task;
struct Task {
    void *(*func)(void *);
    void *arg;
    Task *next;
};

typedef struct {
    Task *task_list;
    pthread_t *workers;
    size_t nworker;
    atomic_bool shutdown;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} ThreadPool;

ThreadPool *thread_pool_create(size_t nworker);
void thread_pool_destroy(ThreadPool *pool);
void thread_pool_add_task(ThreadPool *pool, void *(*func)(void *), void *arg);

#endif // THREAD_POOL_H
