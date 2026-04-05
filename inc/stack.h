#ifndef STACK_H
#define STACK_H

#include "common.h"

#define Stack(T) __Stack_##T

#define DEFINE_STACK(T) \
    typedef struct { \
        T *items; \
        u64 len; \
        u64 cap; \
    } __Stack_##T

#define ALIAS_STACK(old, new) typedef __Stack_##old __Stack_##new

DEFINE_STACK(u64);
ALIAS_STACK(u64, GuestVAddr);

#define stack_push(stk, item) \
    do { \
        if ((stk)->len + 1 > (stk)->cap) { \
            (stk)->cap = (stk)->cap < 32 ? 32 : (stk)->cap * 2; \
            (stk)->items = realloc((stk)->items, sizeof(*(stk)->items) * (stk)->cap); \
            assert((stk)->items && "run out of memory"); \
        } \
        (stk)->items[(stk)->len++] = (item); \
    } while (0)

#define stack_pop(stk) ((stk)->items[--(stk)->len])

#define stack_swap(stk, i, j) \
    do { \
        u8 *arr = (u8 *) (stk)->items; \
        u64 size = sizeof(*(stk)->items); \
        u64 a = (i) * size; \
        u64 b = (j) * size; \
        for (u64 k = 0; k < size; k++) { \
            u8 tmp = arr[a + k]; \
            arr[a + k] = arr[b + k]; \
            arr[b + k] = tmp; \
        } \
    } while (0)

#define stack_top(stk) ((stk)->items[(stk)->len-1])

#endif // STACK_H
