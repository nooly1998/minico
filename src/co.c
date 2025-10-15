//
// co.c — 实现
//
#include "co.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#ifdef __linux__
# include <sys/mman.h>   /* mprotect() 用来作栈溢出保护，可选 */
#endif

/* === 平台相关：ctx_switch() ========================================= */
__attribute__((naked))
static void ctx_switch(coroutine_ctx_t* from, coroutine_ctx_t* to)
{
#ifdef __x86_64__
    __asm__ volatile(
        /* --------- 如果 from==NULL，直接跳到 load_to（主协程首次恢复） */
        "test  %rdi, %rdi          \n"
        "jz    2f                  \n"

        /* 保存 FS.base → from->tls */
        "movq  %%fs:0, %%rax       \n"
        "movq  %%rax, 24(%%rdi)    \n"

        /* 保存 callee-saved xmm6-xmm15 */
        "sub   $160, %%rsp         \n"
        "movdqa %%xmm6,   0(%%rsp) \n"
        "movdqa %%xmm7,  16(%%rsp) \n"
        "movdqa %%xmm8,  32(%%rsp) \n"
        "movdqa %%xmm9,  48(%%rsp) \n"
        "movdqa %%xmm10, 64(%%rsp) \n"
        "movdqa %%xmm11, 80(%%rsp) \n"
        "movdqa %%xmm12, 96(%%rsp) \n"
        "movdqa %%xmm13,112(%%rsp) \n"
        "movdqa %%xmm14,128(%%rsp) \n"
        "movdqa %%xmm15,144(%%rsp) \n"

        /* 保存通用 callee-saved */
        "push  %%rbp               \n"
        "push  %%rbx               \n"
        "push  %%r12               \n"
        "push  %%r13               \n"
        "push  %%r14               \n"
        "push  %%r15               \n"

        /* from->fp / lr / sp */
        "mov   %%rbp,  8(%%rdi)    \n"
        "leaq  1f(%%rip), %%rax    \n"
        "mov   %%rax, 16(%%rdi)    \n"
        "mov   %%rsp,  0(%%rdi)    \n"

        "2: /* ==============  load_to  ================= */\n"

        /* to 不为 NULL（调度器保证） */
        "mov   0(%%rsi), %%rsp     \n"

        /* restore 通用 callee-saved */
        "pop   %%r15               \n"
        "pop   %%r14               \n"
        "pop   %%r13               \n"
        "pop   %%r12               \n"
        "pop   %%rbx               \n"
        "pop   %%rbp               \n"

        /* restore xmm6-xmm15 */
        "movdqa 144(%%rsp), %%xmm15\n"
        "movdqa 128(%%rsp), %%xmm14\n"
        "movdqa 112(%%rsp), %%xmm13\n"
        "movdqa  96(%%rsp), %%xmm12\n"
        "movdqa  80(%%rsp), %%xmm11\n"
        "movdqa  64(%%rsp), %%xmm10\n"
        "movdqa  48(%%rsp), %%xmm9 \n"
        "movdqa  32(%%rsp), %%xmm8 \n"
        "movdqa  16(%%rsp), %%xmm7 \n"
        "movdqa   0(%%rsp), %%xmm6 \n"
        "add   $160, %%rsp         \n"

        /* restore TLS */
        "mov   24(%%rsi), %%rax    \n"
        "mov   %%rax, %%fs:0       \n"

        /* restore fp/lr */
        "mov   8(%%rsi),  %%rbp    \n"
        "mov   16(%%rsi), %%rax    \n"
        "jmp   *%%rax              \n"

        "1:\n"
        "ret\n"
        :
        :
        : "memory","rax"
    );
#elif __aarch64__
    __asm__ volatile(
        "cbz  x0, 2f               \n"  // 如果 from==NULL，跳到 load_to
        
        "mrs x2, TPIDR_EL0\n"
        "str x2, [x0, #24]\n"

        /* -------- 把 x29 / x30 写进 from->fp / from->lr -------- */
        "str  x29, [x0, #8]\n" /* from->fp = x29 */
        "str  x30, [x0, #16]\n" /* from->lr = x30 */

        /* -------- 保存浮点（v8–v15）: 每次 2×128b = 32 字节 ---- */
        "stp  q8,  q9,  [sp, #-32]!\n"
        "stp  q10, q11, [sp, #-32]!\n"
        "stp  q12, q13, [sp, #-32]!\n"
        "stp  q14, q15, [sp, #-32]!\n"

        /* -------- 保存整型 callee-saved（x19–x29, x30） -------- */
        "stp  x29, x30, [sp, #-16]!\n"
        "stp  x27, x28, [sp, #-16]!\n"
        "stp  x25, x26, [sp, #-16]!\n"
        "stp  x23, x24, [sp, #-16]!\n"
        "stp  x21, x22, [sp, #-16]!\n"
        "stp  x19, x20, [sp, #-16]!\n"

        /* -------- 保存当前栈顶指针 → from->sp  ------------------ */
        "mov  x2, sp\n"
        "str  x2, [x0]\n" /* from->sp = sp */

        "2: /* ==============  load_to  ================= */\n"
        /* -------- 切换到目标协程栈 ------------------------------ */
        "ldr  x2, [x1]\n" /* x2 = to->sp */
        "mov  sp, x2\n"

        /* -------- 恢复整型 callee-saved ------------------------- */
        "ldp  x19, x20, [sp], #16\n"
        "ldp  x21, x22, [sp], #16\n"
        "ldp  x23, x24, [sp], #16\n"
        "ldp  x25, x26, [sp], #16\n"
        "ldp  x27, x28, [sp], #16\n"
        "ldp  x29, x30, [sp], #16\n"

        /* -------- 恢复浮点（v8–v15）----------------------------- */
        "ldp  q14, q15, [sp], #32\n"
        "ldp  q12, q13, [sp], #32\n"
        "ldp  q10, q11, [sp], #32\n"
        "ldp  q8,  q9,  [sp], #32\n"

        /* -------- 恢复 TLS -------------------------------------- */
        "ldr  x2, [x1, #24]\n"
        "msr  TPIDR_EL0, x2\n"

        "ldr  x29, [x1, #8]\n"
        "ldr  x30, [x1, #16]\n"

        "ret\n"
    );
#else
# error "Unsupported arch"
#endif
}

/* === 线程局部变量记录当前协程指针 ============================== */
__thread coroutine_t* co_current = NULL;
/* main 协程也给一份 dummy ctx，保证 switch 时指针非空  */
static coroutine_ctx_t main_ctx;

/* === 内部：trampoline — 协程首次进入 ============================ */
static void co_trampoline(void)
{
    coroutine_t* self = co_current;
    assert(self && self->state == NEW);

    self->state = RUNNING;
    self->entry(self->arg); /* 真正用户函数，跑完则协程结束 */

    self->state = DEAD;
    co_yield(); /* 不再返回 */
}

/* === 工具：对齐到 16 字节 ====================================== */
#define ALIGN_DOWN(p) ((uintptr_t)(p) & ~15ULL)

/* === co_create() ============================================== */
coroutine_t* co_create(void (*entry)(void*), void* arg, size_t stack_sz)
{
    if (!entry)
    {
        errno = EINVAL;
        return NULL;
    }
    if (stack_sz < 4096) stack_sz = 64 * 1024;

    /* 页面对齐分配用户栈 */
    uint8_t* stack;
    if (posix_memalign((void**)&stack, 4096, stack_sz) != 0)
        return NULL;

#ifdef __linux__
    /* 可选：最底 1 页保护，做栈溢出检测 */
    if (mprotect(stack, 4096, PROT_NONE) != 0) {
        free(stack);
        return NULL;
    }
#endif

    coroutine_t* co = (coroutine_t*)calloc(1, sizeof(*co));
    if (co == NULL)
    {
        free(stack);
        return NULL;
    }

    co->stack = stack;
    co->stack_sz = stack_sz;
    co->entry = entry;
    co->arg = arg;
    co->state = NEW;
    co->caller = NULL;

    /* 手动在新栈上构造首次 sp 布局：
     * |—— stack_top
     * |   dummy ret addr  (用于 ctx_switch 返回)
     * |   co_trampoline   (第一次 ret 将跳这里)
     */
    uint8_t* top = stack + stack_sz;
    top = (uint8_t*)ALIGN_DOWN(top);

#ifdef __x86_64__
    // 为 x86_64 预留更多空间，包括 XMM 寄存器保存空间
    top -= 8;                             /* dummy rip */
    *(void**)top = NULL;
    top -= 8;
    *(void**)top = (void*)co_trampoline;  /* lr (=rip) */
    
    // 预留空间给 callee-saved 寄存器
    top -= 6 * 8;  // 6 个通用寄存器 (rbp, rbx, r12-r15)
    top -= 160;    // XMM 寄存器保存空间
#elif __aarch64__
    // 为 ARM64 预留更多空间
    top -= 16; /* space for x29,x30 */
    ((void**)top)[0] = NULL;
    ((void**)top)[1] = (void*)co_trampoline;
    
    // 预留空间给 callee-saved 寄存器
    top -= 6 * 16; // 6 对通用寄存器 (x19-x30)
    top -= 4 * 32; // 4 对浮点寄存器 (q8-q15)
#endif
    
    co->ctx.sp = top;
    co->ctx.fp = NULL;
    co->ctx.lr = (void*)co_trampoline;
    co->ctx.tls = NULL; /* 首次进入不用恢复 TLS */

    return co;
}

/* === co_resume() ============================================== */
void co_resume(coroutine_t* co)
{
    if (!co || co->state == DEAD) return;

    coroutine_t* prev = co_current;
    co->caller = prev; /* 记录谁 resume 的 */
    co_current = co;
    
    // 确保上下文切换时有有效的上下文指针
    coroutine_ctx_t* from_ctx = prev ? &prev->ctx : &main_ctx;
    ctx_switch(from_ctx, &co->ctx);
}

/* === co_yield() =============================================== */
void co_yield(void)
{
    coroutine_t* self = co_current;
    if (!self) return; /* main 协程直接返回 */

    self->state = (self->state == RUNNING) ? SUSPEND : self->state;
    coroutine_t* to = self->caller; /* 回到 resume 它的协程 */
    co_current = to;

    // 确保上下文切换时有有效的上下文指针
    coroutine_ctx_t* to_ctx = to ? &to->ctx : &main_ctx;
    ctx_switch(&self->ctx, to_ctx);
}

/* === co_destroy() ============================================= */
void co_destroy(coroutine_t* co)
{
    if (!co) return;
    
    // 放宽销毁条件，允许销毁任何非 RUNNING 状态的协程
    if (co->state == RUNNING && co == co_current) {
        fprintf(stderr, "Warning: attempting to destroy running coroutine\n");
        return;
    }
    
    // 清理内存前先将指针置零，防止意外访问
    if (co->stack) {
        memset(co->stack, 0, co->stack_sz);
        free(co->stack);
        co->stack = NULL;
    }
    
    memset(co, 0, sizeof(*co));
    free(co);
}