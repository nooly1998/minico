//
// Created by 卫明浩 on 25-7-10.
//
#include "vth_pool.h"
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
void sample_task(void* arg) {
    // 使用intptr_t确保指针和整数转换的可移植性
    intptr_t id = (intptr_t)arg;
    for (int i = 0; i < 3; i++) {
        printf("Task %ld: step %d running on thread %p\n", id, i, (void*)pthread_self());
        usleep(10000); // 模拟I/O或其他耗时操作
        co_yield();
    }
    printf("Task %ld: finished on thread %p\n", id, (void*)pthread_self());
}

int main() {
    const int NUM_THREADS = 4;
    const int NUM_TASKS = 20;

    printf("Initializing thread pool with %d physical threads.\n", NUM_THREADS);
    vtp_t pool;
    vtp_init(&pool, NUM_THREADS);

    printf("Submitting %d virtual tasks...\n", NUM_TASKS);
    for (intptr_t i = 0; i < NUM_TASKS; i++) {
        vtp_submit(&pool, sample_task, (void*)i, 16 * 1024);
    }
    printf("All tasks submitted. The pool is running...\n");

    // 等待一段时间，让任务充分执行。
    // 如果不加sleep，shutdown会立刻开始，但依然会正确等待所有任务完成。
    // sleep(2);

    printf("Shutting down pool...\n");
    vtp_shutdown(&pool);

    printf("Test finished successfully.\n");
    return 0;
}