//
// Created by 卫明浩 on 25-7-10.
//
#include "co.h"
#include <stdio.h>

static void foo(void *arg)
{
    for (int i=0;i<3;i++){
        printf("foo i=%d\n", i);
        co_yield();
    }
}

static void bar(void *arg)
{
    for (int i=0;i<2;i++){
        printf("bar i=%d\n", i);
        co_yield();
    }
}

int main()
{
    coroutine_t *a = co_create(foo, NULL, 0);
    coroutine_t *b = co_create(bar, NULL, 0);

    while (a->state!=DEAD || b->state!=DEAD) {
        if (a->state!=DEAD) co_resume(a);
        if (b->state!=DEAD) co_resume(b);
    }
    co_destroy(a);
    co_destroy(b);
    puts("done");
    return 0;
}