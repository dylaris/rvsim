#ifndef CPU_H
#define CPU_H

#include "reg.h"

#define TRAP_MASK (1 << 4)
#define IS_TRAP(ctl) ((ctl) & TRAP_MASK)
typedef enum {
    FLOW_NONE   = 0,
    FLOW_BRANCH = 1,
    FLOW_JUMP   = 2,
    FLOW_ECALL          = TRAP_MASK | 1,
    FLOW_ILLEGAL_INSTR  = TRAP_MASK | 2,
    FLOW_LOAD_MISALIGN  = TRAP_MASK | 3,
    FLOW_STORE_MISALIGN = TRAP_MASK | 4,
    FLOW_LOAD_FAULT     = TRAP_MASK | 5,
    FLOW_STORE_FAULT    = TRAP_MASK | 6,
    FLOW_CRASH          = TRAP_MASK | 7,
    FLOW_HALT           = TRAP_MASK | 8,
} FlowCtrl;

typedef struct {
    FlowCtrl ctl;
    u64 pc;
} Flow;

typedef struct {
    Flow flow;
    GPR gp_regs[NUM_GPRS];
    FPR fp_regs[NUM_FPRS];
    u64 pc;
} CPUState;

static inline void cpu_set_pc(CPUState *state, u64 pc)
{
    state->pc = pc;
}

static inline u64 cpu_get_pc(CPUState *state)
{
    return state->pc;
}

static inline void cpu_reset_flow(CPUState *state)
{
    state->flow = (Flow) {
        .ctl = FLOW_NONE,
        .pc  = cpu_get_pc(state),
    };
}

static inline void cpu_set_flow_pc(CPUState *state, u64 pc)
{
    state->flow.pc = pc;
}

static inline u64 cpu_get_flow_pc(CPUState *state)
{
    return state->flow.pc;
}

static inline void cpu_increase_flow_pc(CPUState *state, u64 increment)
{
    u64 pc = cpu_get_flow_pc(state);
    cpu_set_flow_pc(state, pc + increment);
}

static inline void cpu_set_flow_ctl(CPUState *state, FlowCtrl ctl)
{
    state->flow.ctl = ctl;
}

static inline FlowCtrl cpu_get_flow_ctl(CPUState *state)
{
    return state->flow.ctl;
}

static inline void cpu_increase_pc(CPUState *state, i64 increment)
{
    u64 pc = cpu_get_pc(state);
    cpu_set_pc(state, pc + increment);
}

static inline void cpu_commit_pc(CPUState *state)
{
    cpu_set_pc(state, cpu_get_flow_pc(state));
}

static inline void cpu_set_gpr(CPUState *state, GPRIndex reg, GPR val)
{
    assert(reg >= 0 && reg < NUM_GPRS);
    if (reg == GPR_ZERO)
        return;
    state->gp_regs[reg] = val;
}

static inline GPR cpu_get_gpr(const CPUState *state, GPRIndex reg)
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

static inline FPR cpu_get_fpr(const CPUState *state, FPRIndex reg)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    return state->fp_regs[reg];
}

static inline void cpu_set_fpr_q(CPUState *state, FPRIndex reg, u64 val)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    state->fp_regs[reg].q = val;
}

static inline u64 cpu_get_fpr_q(const CPUState *state, FPRIndex reg)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    return state->fp_regs[reg].q;
}

static inline void cpu_set_fpr_w(CPUState *state, FPRIndex reg, u32 val)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    state->fp_regs[reg].w = val;
}

static inline u32 cpu_get_fpr_w(const CPUState *state, FPRIndex reg)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    return state->fp_regs[reg].w;
}

static inline void cpu_set_fpr_d(CPUState *state, FPRIndex reg, f64 val)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    state->fp_regs[reg].d = val;
}

static inline f64 cpu_get_fpr_d(const CPUState *state, FPRIndex reg)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    return state->fp_regs[reg].d;
}

static inline void cpu_set_fpr_s(CPUState *state, FPRIndex reg, f32 val)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    state->fp_regs[reg].s = val;
}

static inline f32 cpu_get_fpr_s(const CPUState *state, FPRIndex reg)
{
    assert(reg >= 0 && reg < NUM_FPRS);
    return state->fp_regs[reg].s;
}

#endif // CPU_H
