#ifndef UOP_H
#define UOP_H

#include "common.h"

typedef struct {
    u8 *code;
    u64 length;
    bool patch_imm;
    bool patch_reg;
    u64 patch_offset;
} UOP;

#define t0 rax
#define t1 rbx

#define DEFINE_UOP(name, desc, pimm, preg, poff, ...) \
    static u8 uop_##name##_code[] = { __VA_ARGS__ }; \
    static UOP uop_##name[] = { \
        .code = uop_##name##_code, \
        .length = sizeof(uop_##name##_code), \
        .patch_imm = (pimm), \
        .patch_reg = (preg), \
        .patch_offset = (poff), \
    };

// move
DEFINE_UOP(move_imm_t0, "t0 = imm", true,  false, 0, 0xff, 0xff)
DEFINE_UOP(move_imm_t1, "t1 = imm", true,  false, 0, 0xff, 0xff)
DEFINE_UOP(move_gpr_t0, "t0 = gpr", false, true,  0, 0xff, 0xff)
DEFINE_UOP(move_gpr_t1, "t1 = gpr", false, true,  0, 0xff, 0xff)
DEFINE_UOP(move_t0_gpr, "gpr = t0", false, true,  0, 0xff, 0xff)
DEFINE_UOP(move_t1_gpr, "gpr = t1", false, true,  0, 0xff, 0xff)
DEFINE_UOP(move_t0_t1,  "t1 = t0", false, false, 0, 0xff, 0xff)
DEFINE_UOP(move_t1_t0,  "t0 = t1", false, false, 0, 0xff, 0xff)

// arith
DEFINE_UOP(add_t0_t1,  "t0 = t0 + t1", false, false, 0, 0xff, 0xff)
DEFINE_UOP(sub_t0_t1,  "t0 = t0 - t1", false, false, 0, 0xff, 0xff)
DEFINE_UOP(mul_t0_t1,  "t0 = t0 * t1", false, false, 0, 0xff, 0xff)
DEFINE_UOP(div_t0_t1,  "t0 = t0 / t1", false, false, 0, 0xff, 0xff)
DEFINE_UOP(rem_t0_t1,  "t0 = t0 % t1", false, false, 0, 0xff, 0xff)
DEFINE_UOP(sra_t0_t1,  "t0 = t0 >> t1", false, false, 0, 0xff, 0xff)

// logic
DEFINE_UOP(and_t0_t1,  "t0 = t0 & t1", false, false, 0, 0xff, 0xff)
DEFINE_UOP(or_t0_t1,   "t0 = t0 | t1", false, false, 0, 0xff, 0xff)
DEFINE_UOP(xor_t0_t1,  "t0 = t0 ^ t1", false, false, 0, 0xff, 0xff)
DEFINE_UOP(sll_t0_t1,  "t0 = t0 << t1", false, false, 0, 0xff, 0xff)
DEFINE_UOP(srl_t0_t1,  "t0 = t0 >> t1", false, false, 0, 0xff, 0xff)

// compare
DEFINE_UOP(eq_t0_t1,  "t0 = t0 == t1", false, false, 0, 0xff, 0xff)
DEFINE_UOP(ne_t0_t1,  "t0 = t0 != t1", false, false, 0, 0xff, 0xff)
DEFINE_UOP(lt_t0_t1,  "t0 = t0 <  t1", false, false, 0, 0xff, 0xff)
DEFINE_UOP(ge_t0_t1,  "t0 = t0 >= t1", false, false, 0, 0xff, 0xff)

// load
DEFINE_UOP(load_s8_t0,  "t0 = [t0 + off]", false, false, 0, 0xff, 0xff)
DEFINE_UOP(load_s16_t0, "t0 = [t0 + off]", false, false, 0, 0xff, 0xff)
DEFINE_UOP(load_s32_t0, "t0 = [t0 + off]", false, false, 0, 0xff, 0xff)
DEFINE_UOP(load_u8_t0,  "t0 = [t0 + off]", false, false, 0, 0xff, 0xff)
DEFINE_UOP(load_u16_t0, "t0 = [t0 + off]", false, false, 0, 0xff, 0xff)
DEFINE_UOP(load_u32_t0, "t0 = [t0 + off]", false, false, 0, 0xff, 0xff)
DEFINE_UOP(load_u64_t0, "t0 = [t0 + off]", false, false, 0, 0xff, 0xff)

// store
DEFINE_UOP(store_u8_t0,  "[t0 + off] = t0", false, false, 0, 0xff, 0xff)
DEFINE_UOP(store_u16_t0, "[t0 + off] = t0", false, false, 0, 0xff, 0xff)
DEFINE_UOP(store_u32_t0, "[t0 + off] = t0", false, false, 0, 0xff, 0xff)
DEFINE_UOP(store_u64_t0, "[t0 + off] = t0", false, false, 0, 0xff, 0xff)

// branch

// jump

#endif // UOP_H
