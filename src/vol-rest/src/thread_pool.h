#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <stdbool.h>

/*
 * Thread Pool.
 * by fsc
 */


// task_job
typedef struct {
    void (*function)(void*);
    void* argument;
} threadpool_task_t;

// thread_pool struct
typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    threadpool_task_t *task_queue;
    int thread_count;
    int queue_size;
    int head;
    int tail;
    int count;
    int shutdown;
    int started;
} threadpool_t;


// thread_pool function 
// init thread_pool
threadpool_t *threadpool_create(int thread_count, int queue_size);
// push tasks into thread pool
int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument);
// destory thread pool
int threadpool_destroy(threadpool_t *pool);

#endif // THREADPOOL_H
