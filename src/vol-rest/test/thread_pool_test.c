#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../src/thread_pool.h"

#define THREAD_COUNT 4
#define TASK_COUNT 4
#define TASK_REPEAT_COUNT 3

int tasks_completed = 0;
threadpool_t *pool;

// 模拟任务函数
void dummy_task(void *arg) {
    int task_id = *((int *)arg);
    printf("Task %d is being processed by thread %ld\n", task_id, pthread_self());
    usleep(500000); // 模拟任务处理时间

    pthread_mutex_lock(&pool->lock);
    tasks_completed++;
    printf("Task %d complete: %d \n",task_id,tasks_completed);
    if (tasks_completed % TASK_COUNT == 0) {
        pthread_cond_signal(&pool->notify);
    }
    pthread_mutex_unlock(&pool->lock);
}

int main() {
    // 创建线程池
    pool = threadpool_create(THREAD_COUNT, TASK_COUNT);
    if (pool == NULL) {
        fprintf(stderr, "Failed to create thread pool\n");
        return EXIT_FAILURE;
    }

    // 提交任务到线程池
    int task_args[TASK_COUNT];
    for (int j = 0; j < TASK_REPEAT_COUNT; ++j) {
        tasks_completed=0;

        for (int i = 0; i < TASK_COUNT; ++i) {
            task_args[i] = i;
            threadpool_add(pool, dummy_task, (void *)&task_args[i]);
        }

        // 等待所有任务完成
        pthread_mutex_lock(&pool->lock);
        while (tasks_completed < TASK_COUNT) {
            pthread_cond_wait(&pool->notify, &pool->lock);
        }
        pthread_mutex_unlock(&pool->lock);
           // 等待所有线程完成本轮任务
        // for (int i = 0; i < THREAD_COUNT; ++i) {
        //     pthread_join(pool->threads[i], NULL);
        // }
        // 打印任务完成情况
        printf("All tasks completed for repeat count %d.\n", j + 1);
    }
    
    // 销毁线程池
    threadpool_destroy(pool);

    return EXIT_SUCCESS;
}
