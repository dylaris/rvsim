#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

static void *worker_func(void *arg)
{
    ThreadPool *pool = (ThreadPool *) arg;

    while (true) {
        pthread_mutex_lock(&pool->mutex);

        // Wait for task
        while (!pool->task_list && !atomic_load(&pool->shutdown)) {
            pthread_cond_wait(&pool->not_empty, &pool->mutex);
        }

        // Check if the pool shutdown
        if (atomic_load(&pool->shutdown)) {
            pthread_mutex_unlock(&pool->mutex);
            pthread_exit(NULL);
            break;
        }

        // Get the task from task list
        Task *task = pool->task_list;
        pool->task_list = task->next;

        pthread_mutex_unlock(&pool->mutex);

        // Execute the task
        // printf("do task\n");
        task->func(task->arg);
        free(task);
    }

    return NULL;
}

ThreadPool *thread_pool_create(size_t nworker)
{
    ThreadPool *pool = malloc(sizeof(ThreadPool));
    assert(pool && "run out of memory");

    pool->task_list = NULL;

    pool->workers = malloc(sizeof(pthread_t) * nworker);
    assert(pool->workers && "run out of memory");
    pool->nworker = nworker;

    atomic_store(&pool->shutdown, false);

    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->not_empty, NULL);

    for (size_t i = 0; i < nworker; i++) {
        pthread_create(&pool->workers[i], NULL, worker_func, pool);
    }

    return pool;
}

void thread_pool_destroy(ThreadPool *pool)
{
    atomic_store(&pool->shutdown, true);

    // Awake up all worker threads to check the shutdown flag and exit
    pthread_cond_broadcast(&pool->not_empty);

    // Wait until all threads exit
    for (size_t i = 0; i < pool->nworker; i++) {
        pthread_join(pool->workers[i], NULL);
    }

    // Free all uncompleted tasks
    pthread_mutex_lock(&pool->mutex);
    while (pool->task_list) {
        Task *task = pool->task_list;
        pool->task_list = task->next;
        free(task);
    }
    pthread_mutex_unlock(&pool->mutex);

    free(pool->workers);
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->not_empty);
    free(pool);
}

void thread_pool_add_task(ThreadPool *pool, void *(*func)(void *), void *arg)
{
    if (atomic_load(&pool->shutdown)) return;

    pthread_mutex_lock(&pool->mutex);
    Task *new_task = malloc(sizeof(Task));
    assert(pool && "run out of memory");
    new_task->func = func;
    new_task->arg = arg;
    new_task->next = pool->task_list;
    pool->task_list = new_task;
    pthread_mutex_unlock(&pool->mutex);

    pthread_cond_signal(&pool->not_empty);
}
