    .text
    .globl ctx_switch
    .type  ctx_switch, @function
ctx_switch:
    /* 保存 callee-saved xmm6-xmm15 (160 bytes) */
    subq   $160, %rsp
    movdqu %xmm6,   0(%rsp)
    movdqu %xmm7,  16(%rsp)
    movdqu %xmm8,  32(%rsp)
    movdqu %xmm9,  48(%rsp)
    movdqu %xmm10, 64(%rsp)
    movdqu %xmm11, 80(%rsp)
    movdqu %xmm12, 96(%rsp)
    movdqu %xmm13,112(%rsp)
    movdqu %xmm14,128(%rsp)
    movdqu %xmm15,144(%rsp)

    /* 保存通用 callee-saved */
    pushq  %rbp
    pushq  %rbx
    pushq  %r12
    pushq  %r13
    pushq  %r14
    pushq  %r15

    /* if (from != NULL) save to *from */
    testq  %rdi, %rdi
    jz     2f

    movq   %rbp,  8(%rdi)
    leaq   1f(%rip), %rax
    movq   %rax, 16(%rdi)
    movq   %rsp,  0(%rdi)

2:
    /* 切换到目标协程栈 */
    movq   0(%rsi), %rsp

    /* 恢复通用寄存器 */
    popq   %r15
    popq   %r14
    popq   %r13
    popq   %r12
    popq   %rbx
    popq   %rbp

    /* 恢复 xmm6-xmm15 */
    movdqu 144(%rsp), %xmm15
    movdqu 128(%rsp), %xmm14
    movdqu 112(%rsp), %xmm13
    movdqu  96(%rsp), %xmm12
    movdqu  80(%rsp), %xmm11
    movdqu  64(%rsp), %xmm10
    movdqu  48(%rsp), %xmm9
    movdqu  32(%rsp), %xmm8
    movdqu  16(%rsp), %xmm7
    movdqu   0(%rsp), %xmm6
    addq   $160, %rsp

    /* 恢复 fp/lr 并跳转 */
    movq   8(%rsi), %rbp
    movq   16(%rsi), %rax
    jmp    *%rax

1:
    retq
    .size ctx_switch, .-ctx_switch
