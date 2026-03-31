#ifndef CPU_H
#define CPU_H

#include "common.h"

typedef enum {
    GPR_ZERO = 0, GPR_RA, GPR_SP, GPR_GP, GPR_TP,
    GPR_T0, GPR_T1, GPR_T2,
    GPR_S0, GPR_S1,
    GPR_A0, GPR_A1, GPR_A2, GPR_A3, GPR_A4, GPR_A5, GPR_A6, GPR_A7,
    GPR_S2, GPR_S3, GPR_S4, GPR_S5, GPR_S6, GPR_S7, GPR_S8, GPR_S9, GPR_S10, GPR_S11,
    GPR_T3, GPR_T4, GPR_T5, GPR_T6,
    NUM_GPRS,
} GPRIndex;

typedef enum {
    FPR_FT0 = 0, FPR_FT1, FPR_FT2, FPR_FT3, FPR_FT4, FPR_FT5, FPR_FT6, FPR_FT7,
    FPR_FS0, FPR_FS1,
    FPR_FA0, FPR_FA1, FPR_FA2, FPR_FA3, FPR_FA4, FPR_FA5, FPR_FA6, FPR_FA7,
    FPR_FS2, FPR_FS3, FPR_FS4, FPR_FS5, FPR_FS6, FPR_FS7, FPR_FS8, FPR_FS9, FPR_FS10, FPR_FS11,
    FPR_FT8, FPR_FT9, FPR_FT10, FPR_FT11,
    NUM_FPRS,
} FPRIndex;

typedef enum {
    BLOCK_NONE = 0,
    BLOCK_JUMP,
    BLOCK_ECALL,
    BLOCK_ERROR,
} BlockExitReason;

typedef enum {
    ERROR_NONE = 0,
    ERROR_DECODE,
    ERROR_INTERP,
} ErrorCode;

typedef struct {
    BlockExitReason exit_reason;
    u64 target_pc;
    u32 raw_data;
    ErrorCode error_code;
} Trace;

typedef u64 GPR;
typedef union {
    u64 q;
    f64 d;
    u32 w;
    f32 s;
} FPR;

typedef struct {
    Trace trace;
    GPR gp_regs[NUM_GPRS];
    FPR fp_regs[NUM_FPRS];
    u64 pc;
} CPUState;

static inline void cpu_clean_trace(CPUState *state)
{
    state->trace = (Trace) {
        .exit_reason = BLOCK_NONE,
        .target_pc = 0,
        .raw_data = 0,
        .error_code = ERROR_NONE,
    };
}

static inline void cpu_set_gpr(CPUState *state, GPRIndex reg, GPR val)
{
    assert(reg >= 0 && reg < NUM_GPRS);
    if (reg == GPR_ZERO)
        return;
    state->gp_regs[reg] = val;
}

static inline GPR cpu_get_gpr(CPUState *state, GPRIndex reg)
{
    assert(reg >= 0 && reg < NUM_GPRS);
    if (reg == GPR_ZERO)
        return 0;
    return state->gp_regs[reg];
}

static inline void cpu_set_fpr(CPUState *state, FPRIndex reg, FPR val)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    state->fp_regs[reg] = val;
}

static inline FPR cpu_get_fpr(CPUState *state, FPRIndex reg)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    return state->fp_regs[reg];
}

static inline void cpu_set_fpr_q(CPUState *state, FPRIndex reg, u64 val)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    state->fp_regs[reg].q = val;
}

static inline u64 cpu_get_fpr_q(CPUState *state, FPRIndex reg)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    return state->fp_regs[reg].q;
}

static inline void cpu_set_fpr_w(CPUState *state, FPRIndex reg, u32 val)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    state->fp_regs[reg].w = val;
}

static inline u32 cpu_get_fpr_w(CPUState *state, FPRIndex reg)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    return state->fp_regs[reg].w;
}

static inline void cpu_set_fpr_d(CPUState *state, FPRIndex reg, f64 val)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    state->fp_regs[reg].d = val;
}

static inline f64 cpu_get_fpr_d(CPUState *state, FPRIndex reg)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    return state->fp_regs[reg].d;
}

static inline void cpu_set_fpr_s(CPUState *state, FPRIndex reg, f32 val)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    state->fp_regs[reg].s = val;
}

static inline f32 cpu_get_fpr_s(CPUState *state, FPRIndex reg)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    return state->fp_regs[reg].s;
}

#endif // CPU_H
