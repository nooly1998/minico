//
// Created by 卫明浩 on 25-7-9.
//

#ifndef VTH_POOL_H
#define VTH_POOL_H

#include <pthread.h>
#include <stdbool.h>

#include "co.h"

/* 单个虚拟任务 —— 本质就是一个 coroutine_t */
typedef struct vtask {
    coroutine_t      *co;
    struct vtask     *next;
} vtask_t;

/* 线程池 */
typedef struct vtp {
    size_t            nthreads;
    pthread_t        *threads;

    vtask_t          *q_head;          /* 任务队列 */
    vtask_t          *q_tail;
    pthread_mutex_t   q_mtx;
    pthread_cond_t    q_cv;

    bool              stop;            /* 关机标志 */
} vtp_t;

extern __thread coroutine_t *co_current;

void vtp_init(vtp_t *p, size_t n);
void vtp_shutdown(vtp_t *p);
void vtp_submit(vtp_t *p, void (*entry)(void *), void *arg, size_t stack_sz);
void co_yield(void);

#endif //VTH_POOL_H
