//
// Created by 卫明浩 on 25-7-10.
//

#ifndef CO_H
#define CO_H

#include <stdint.h>
#include <stdlib.h>

typedef enum { NEW, RUNNING, SUSPEND, DEAD } co_state;

/* 平台无关的寄存器快照 */
typedef struct coroutine_ctx {
    void *sp;     /* 栈指针 */
    void *fp;     /* x86-64: rbp│AArch64: x29 */
    void *lr;     /* x86-64: rip│AArch64: x30 */
    void *tls;    /* 保存 / 恢复 FS.base│TPIDR_EL0 */
} coroutine_ctx_t;

typedef struct coroutine {
    coroutine_ctx_t ctx;
    uint8_t        *stack;  /* malloc 得到的用户栈底 */
    size_t          stack_sz;
    void          (*entry)(void *);
    void           *arg;
    co_state        state;
    struct coroutine *caller; /* resume 它的协程，yield 时回退 */
} coroutine_t;

/* API */
coroutine_t *co_create(void (*entry)(void *), void *arg,
                       size_t stack_sz /*>=4K, 默认 64K*/);
void         co_resume(coroutine_t *co);
void         co_yield(void);
void         co_destroy(coroutine_t *co);

#endif //CO_H
