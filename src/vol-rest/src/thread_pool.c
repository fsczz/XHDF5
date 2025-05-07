#include "thread_pool.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static void *threadpool_thread(void *threadpool);

threadpool_t *threadpool_create(int thread_count, int queue_size) {
    threadpool_t *pool;
    int i;

    if ((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) {
        return NULL;
    }

    pool->thread_count = thread_count;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = pool->shutdown = pool->started = 0;

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    pool->task_queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size);

    if ((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
        (pthread_cond_init(&(pool->notify), NULL) != 0) ||
        (pool->threads == NULL) ||
        (pool->task_queue == NULL)) {
        return NULL;
    }

    for (i = 0; i < thread_count; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void*)pool) != 0) {
            threadpool_destroy(pool);
            return NULL;
        }
        pool->started++;
    }

    return pool;
}

int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument) {
    int next, err = 0;

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return -1;
    }

    next = (pool->tail + 1) % pool->queue_size;

    do {
        if (pool->count == pool->queue_size) {
            err = -1;
            break;
        }

        pool->task_queue[pool->tail].function = function;
        pool->task_queue[pool->tail].argument = argument;
        pool->tail = next;
        pool->count += 1;

        if (pthread_cond_signal(&(pool->notify)) != 0) {
            err = -1;
            break;
        }
    } while (0);

    if (pthread_mutex_unlock(&pool->lock) != 0) {
        err = -1;
    }

    return err;
}

int threadpool_destroy(threadpool_t *pool) {
    int i, err = 0;

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return -1;
    }

    pool->shutdown = 1;

    if ((pthread_cond_broadcast(&(pool->notify)) != 0) ||
        (pthread_mutex_unlock(&(pool->lock)) != 0)) {
        return -1;
    }

    for (i = 0; i < pool->thread_count; i++) {
        if (pthread_join(pool->threads[i], NULL) != 0) {
            err = -1;
        }
    }

    if (err == 0) {
        if (pthread_mutex_destroy(&(pool->lock)) != 0) {
            err = -1;
        }
        if (pthread_cond_destroy(&(pool->notify)) != 0) {
            err = -1;
        }
    }

    if (err == 0) {
        free(pool->threads);
        free(pool->task_queue);
        free(pool);
    }

    return err;
}

static void *threadpool_thread(void *threadpool) {
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;

    while (1) {
        pthread_mutex_lock(&(pool->lock));

        while ((pool->count == 0) && (!pool->shutdown)) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if ((pool->shutdown) && (pool->count == 0)) {
            break;
        }

        task.function = pool->task_queue[pool->head].function;
        task.argument = pool->task_queue[pool->head].argument;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count -= 1;

        pthread_mutex_unlock(&(pool->lock));

        (*(task.function))(task.argument);
    }

    pool->started--;

    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
    return NULL;
}
