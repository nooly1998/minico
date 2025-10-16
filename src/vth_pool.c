//
// Created by 卫明浩 on 25-7-9.
//

#include "vth_pool.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

static void *vtp_worker(void *arg) {
    vtp_t *p = arg;

    coroutine_t *sched_co = malloc(sizeof(coroutine_t));
    if (!sched_co) {
        fprintf(stderr, "Failed to allocate scheduler coroutine\n");
        return NULL;
    }

    memset(sched_co, 0, sizeof(coroutine_t));
    sched_co->state = RUNNING; // 调度器协程永远是 RUNNING 状态

    // 将当前线程的上下文正确地设置为调度器协程
    co_current = sched_co;

    for (;;)
    {
        pthread_mutex_lock(&p->q_mtx);
        while (p->q_head == NULL && !p->stop)
            pthread_cond_wait(&p->q_cv, &p->q_mtx);

        if (p->stop && p->q_head == NULL) {
            pthread_mutex_unlock(&p->q_mtx);
            break; // 退出循环，在函数末尾统一清理
        }

        vtask_t *vt = p->q_head;
        p->q_head = vt->next;
        if (p->q_head == NULL) p->q_tail = NULL;
        pthread_mutex_unlock(&p->q_mtx);

        // 验证任务有效性
        if (!vt || !vt->co) {
            if (vt) {
                if (vt->co) co_destroy(vt->co);
                free(vt);
            }
            continue;
        }

        // 额外的安全检查：确保协程处于有效状态
        if (vt->co->state != NEW && vt->co->state != SUSPEND) {
            fprintf(stderr, "Warning: attempting to resume coroutine in invalid state %d\n", vt->co->state);
            co_destroy(vt->co);
            free(vt);
            continue;
        }

        // co_resume 现在可以正确地将 co_current (sched_co) 保存为 vt->co 的调用者
        co_resume(vt->co);

        // 从任务协程 co_yield() 返回后，co_current 会被正确地恢复为 sched_co
        // 添加验证确保 co_current 正确恢复
        if (co_current != sched_co) {
            fprintf(stderr, "Warning: co_current not properly restored after yield\n");
            co_current = sched_co;
        }

        if (vt->co->state == DEAD)
        {
            co_destroy(vt->co);
            free(vt);
        }
        else if (vt->co->state == SUSPEND) // 任务被挂起，需要重新入队
        {
            pthread_mutex_lock(&p->q_mtx);
            if (!p->stop) {  // 只有在没有停止的情况下才重新入队
                vt->next = NULL;
                if (p->q_tail) {
                    p->q_tail->next = vt;
                } else {
                    p->q_head = vt;
                }
                p->q_tail = vt;
                pthread_cond_signal(&p->q_cv);
            } else {
                // 如果已经停止，直接销毁任务
                co_destroy(vt->co);
                free(vt);
            }
            pthread_mutex_unlock(&p->q_mtx);
        }
        else
        {
            // 处理异常状态
            fprintf(stderr, "Warning: task in unexpected state %d\n", vt->co->state);
            co_destroy(vt->co);
            free(vt);
        }
    }

    // 清理：释放调度器协程对象
    co_current = NULL;
    free(sched_co);
    return NULL;
}

void vtp_init(vtp_t* p, size_t n) {
    memset(p, 0, sizeof(*p));
    p->threads = calloc(n, sizeof(pthread_t));
    if (!p->threads) {
        fprintf(stderr, "Failed to allocate threads array\n");
        return;
    }

    pthread_mutex_init(&p->q_mtx, NULL);
    pthread_cond_init(&p->q_cv, NULL);

    size_t created = 0;
    for (size_t i = 0; i < n; ++i) {
        pthread_t tid;
        if (pthread_create(&tid, NULL, vtp_worker, p) == 0) {
            p->threads[created++] = tid;
        } else {
            fprintf(stderr, "Failed to create worker thread %zu\n", i);
        }
    }
    p->nthreads = created;

    if (created == 0) {
        fprintf(stderr, "Fatal: no worker threads created\n");
        pthread_mutex_destroy(&p->q_mtx);
        pthread_cond_destroy(&p->q_cv);
        free(p->threads);
        p->threads = NULL;
    } else if (created < n) {
        fprintf(stderr, "Warning: only %zu of %zu threads were created\n",
                created, n);
    }
}

void vtp_shutdown(vtp_t *p) {
    // 设置停止标志并通知所有工作线程
    pthread_mutex_lock(&p->q_mtx);
    p->stop = true;
    pthread_cond_broadcast(&p->q_cv);
    pthread_mutex_unlock(&p->q_mtx);

    // 等待所有线程结束
    for (size_t i = 0; i < p->nthreads; ++i) {
        pthread_join(p->threads[i], NULL);
    }

    // 清理队列中可能残留的任务
    pthread_mutex_lock(&p->q_mtx);
    vtask_t *vt = p->q_head;
    int orphaned_tasks = 0;
    while (vt) {
        vtask_t *next = vt->next;
        if (vt->co) {
            if (vt->co->state != DEAD) {
                orphaned_tasks++;
            }
            co_destroy(vt->co);
        }
        free(vt);
        vt = next;
    }
    if (orphaned_tasks > 0) {
        fprintf(stderr, "Warning: %d tasks were terminated before completion\n",
                orphaned_tasks);
    }
    p->q_head = p->q_tail = NULL;
    pthread_mutex_unlock(&p->q_mtx);

    // 清理资源
    free(p->threads);
    p->threads = NULL;
    pthread_mutex_destroy(&p->q_mtx);
    pthread_cond_destroy(&p->q_cv);
}

void vtp_submit(vtp_t *p, void (*entry)(void *), void *arg, size_t stack_sz) {
    if (!p || !entry) return;

    // 先创建资源
    coroutine_t *co = co_create(entry, arg, stack_sz);
    if (!co) {
        fprintf(stderr, "Failed to create coroutine\n");
        return;
    }

    vtask_t *vt = malloc(sizeof(*vt));
    if (!vt) {
        co_destroy(co);
        fprintf(stderr, "Failed to allocate task\n");
        return;
    }
    vt->co = co;
    vt->next = NULL;

    // 一次性检查并入队
    pthread_mutex_lock(&p->q_mtx);
    if (p->stop) {
        pthread_mutex_unlock(&p->q_mtx);
        co_destroy(co);
        free(vt);
        return;
    }

    if (p->q_tail) {
        p->q_tail->next = vt;
    } else {
        p->q_head = vt;
    }
    p->q_tail = vt;
    pthread_cond_signal(&p->q_cv);
    pthread_mutex_unlock(&p->q_mtx);
}
