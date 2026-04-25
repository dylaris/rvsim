#ifndef INTERP_H
#define INTERP_H

#include <math.h>
#include "machine.h"

void interp(Machine *machine);

/*
 * Help functions below
 */

static inline u64 help_mulhu(u64 a, u64 b)
{
    u64 t;
    uint32_t y1, y2, y3;
    u64 a0 = (uint32_t)a, a1 = a >> 32;
    u64 b0 = (uint32_t)b, b1 = b >> 32;
    t = a1*b0 + ((a0*b0) >> 32);
    y1 = t;
    y2 = t >> 32;
    t = a0*b1 + y1;
    y1 = t;
    t = a1*b1 + y2 + (t >> 32);
    y2 = t;
    y3 = t >> 32;
    return ((u64)y3 << 32) | y2;
}

static inline i64 help_mulh(i64 a, i64 b)
{
    int negate = (a < 0) != (b < 0);
    u64 res = help_mulhu(a < 0 ? -a : a, b < 0 ? -b : b);
    return negate ? ~res + (a * b == 0) : res;
}

static inline i64 help_mulhsu(i64 a, i64 b)
{
    int negate = a < 0;
    u64 res = help_mulhu(a < 0 ? -a : a, b);
    return negate ? ~res + (a * b == 0) : res;
}

#if 1

#define help_div(a, b) (((b) == 0) ? (i64)-1  : (((i64)(a) == INT64_MIN && (i64)(b) == (i64)-1) ? INT64_MIN : (i64)(a) / (i64)(b)))
#define help_rem(a, b) (((b) == 0) ? (i64)(a) : (((i64)(a) == INT64_MIN && (i64)(b) == (i64)-1) ? (i64)0    : (i64)(a) % (i64)(b)))

#else

static inline i64 help_div(i64 a, i64 b)
{
    if (b == 0)
        return -1;
    else if (a == INT64_MIN && b == -1)
        return INT64_MIN;
    else
        return a / b;
}

static inline i64 help_rem(i64 a, i64 b)
{
    if (b == 0)
        return a;
    else if (a == INT64_MIN && b == -1)
        return 0;
    else
        return a % b;
}

#endif

#define F32_SIGN ((uint32_t)1 << 31)
#define F64_SIGN ((u64)1 << 63)

static inline u32 help_fsgnj32(u32 a, u32 b, bool n, bool x)
{
    u32 v = x ? a : n ? F32_SIGN : 0;
    return (a & ~F32_SIGN) | ((v ^ b) & F32_SIGN);
}

static inline u64 help_fsgnj64(u64 a, u64 b, bool n, bool x)
{
    u64 v = x ? a : n ? F64_SIGN : 0;
    return (a & ~F64_SIGN) | ((v ^ b) & F64_SIGN);
}

static inline u64 help_fsgnj(u64 a, u64 b, u64 size, bool neg, bool xor)
{
    if (size == 8)
        return help_fsgnj64(a, b, neg, xor);
    else if (size == 4)
        return help_fsgnj32(a, b, neg, xor);
    else
        fatal("size must be 4 or 8");
}

union u32_f32 { u32 ui; f32 f; };
#define signF32UI(a) ((bool) ((uint32_t) (a)>>31))
#define expF32UI(a) ((int_fast16_t) ((a)>>23) & 0xFF)
#define fracF32UI(a) ((a) & 0x007FFFFF)
#define isNaNF32UI(a) (((~(a) & 0x7F800000) == 0) && ((a) & 0x007FFFFF))
#define isSigNaNF32UI(uiA) ((((uiA) & 0x7FC00000) == 0x7F800000) && ((uiA) & 0x003FFFFF))

static inline u16 help_f32_classify(f32 a)
{
    union u32_f32 uA;
    u32 uiA;

    uA.f = a;
    uiA = uA.ui;

    u16 infOrNaN = expF32UI(uiA) == 0xFF;
    u16 subnormalOrZero = expF32UI(uiA) == 0;
    bool sign = signF32UI(uiA);
    bool fracZero = fracF32UI(uiA) == 0;
    bool isNaN = isNaNF32UI(uiA);
    bool isSNaN = isSigNaNF32UI(uiA);

    return
        (sign && infOrNaN && fracZero)          << 0 |
        (sign && !infOrNaN && !subnormalOrZero) << 1 |
        (sign && subnormalOrZero && !fracZero)  << 2 |
        (sign && subnormalOrZero && fracZero)   << 3 |
        (!sign && infOrNaN && fracZero)          << 7 |
        (!sign && !infOrNaN && !subnormalOrZero) << 6 |
        (!sign && subnormalOrZero && !fracZero)  << 5 |
        (!sign && subnormalOrZero && fracZero)   << 4 |
        (isNaN &&  isSNaN)                       << 8 |
        (isNaN && !isSNaN)                       << 9;
}

union u64_f64 { u64 ui; f64 f; };
#define signF64UI(a) ((bool) ((u64) (a)>>63))
#define expF64UI(a) ((int_fast16_t) ((a)>>52) & 0x7FF)
#define fracF64UI(a) ((a) & UINT64_C(0x000FFFFFFFFFFFFF))
#define isNaNF64UI(a) (((~(a) & UINT64_C(0x7FF0000000000000)) == 0) && ((a) & UINT64_C(0x000FFFFFFFFFFFFF)))
#define isSigNaNF64UI(uiA) ((((uiA) & UINT64_C(0x7FF8000000000000)) == UINT64_C(0x7FF0000000000000)) && ((uiA) & UINT64_C(0x0007FFFFFFFFFFFF)))

static inline u16 help_f64_classify(f64 a)
{
    union u64_f64 uA;
    u64 uiA;

    uA.f = a;
    uiA = uA.ui;

    u16 infOrNaN = expF64UI(uiA) == 0x7FF;
    u16 subnormalOrZero = expF64UI(uiA) == 0;
    bool sign = signF64UI(uiA);
    bool fracZero = fracF64UI(uiA) == 0;
    bool isNaN = isNaNF64UI(uiA);
    bool isSNaN = isSigNaNF64UI(uiA);

    return
        (sign && infOrNaN && fracZero)          << 0 |
        (sign && !infOrNaN && !subnormalOrZero) << 1 |
        (sign && subnormalOrZero && !fracZero)  << 2 |
        (sign && subnormalOrZero && fracZero)   << 3 |
        (!sign && infOrNaN && fracZero)          << 7 |
        (!sign && !infOrNaN && !subnormalOrZero) << 6 |
        (!sign && subnormalOrZero && !fracZero)  << 5 |
        (!sign && subnormalOrZero && fracZero)   << 4 |
        (isNaN &&  isSNaN)                       << 8 |
        (isNaN && !isSNaN)                       << 9;
}

static inline u16 help_f_classify(f64 a, u64 size)
{
    if (size == 8)
        return help_f64_classify(a);
    else if (size == 4)
        return help_f32_classify(a);
    else
        fatal("size must be 4 or 8");
}

#endif // INTERP_H
