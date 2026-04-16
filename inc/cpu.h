#ifndef CPU_H
#define CPU_H

#include "reg.h"

#define TRAP_MASK (1 << 4)
#define IS_TRAP(ctl) ((ctl) & TRAP_MASK)
typedef enum {
    FLOW_NONE           = 0,
    FLOW_BRANCH         = 1,
    FLOW_JUMP           = 2,
    FLOW_SKIP_CODEGEN   = 3,
    FLOW_ECALL          = TRAP_MASK | 1,
    FLOW_ILLEGAL_INSTR  = TRAP_MASK | 2,
    FLOW_LOAD_MISALIGN  = TRAP_MASK | 3,
    FLOW_STORE_MISALIGN = TRAP_MASK | 4,
    FLOW_LOAD_FAULT     = TRAP_MASK | 5,
    FLOW_STORE_FAULT    = TRAP_MASK | 6,
    FLOW_CRASH          = TRAP_MASK | 7,
    FLOW_HALT           = TRAP_MASK | 8,
    FLOW_CACHE_OVERFLOW = TRAP_MASK | 9,
} FlowCtrl;

typedef struct {
    u64 pc;
    FlowCtrl ctl;
} Flow;

/*
 * Within the interpreter loop:
 * - The variable 'pc' points to the instruction currently being processed.
 * - The variable 'flow.pc' is pre-calculated to point to the subsequent instruction.
 * To maintain correct execution state, the value of 'pc' must be synchronized
 * with 'flow.pc' at the end of processing the current instruction. (call cpu_commit_pc())
 */

typedef struct {
    GPR gp_regs[NUM_GPRS];
    u64 pc;
    Flow flow;
    FPR fp_regs[NUM_FPRS];
} CPUState;

#define cpu_set_pc(state_, pc_)                ((state_)->pc = (pc_))
#define cpu_get_pc(state_)                     ((state_)->pc)

#define cpu_reset_flow_ctl(state_)             ((state_)->flow.ctl = FLOW_NONE)
#define cpu_set_flow_pc(state_, pc_)           ((state_)->flow.pc = (pc_))
#define cpu_get_flow_pc(state_)                ((state_)->flow.pc)
#define cpu_increase_flow_pc(state_, inc_)     cpu_set_flow_pc(state_, cpu_get_flow_pc(state_) + (inc_))
#define cpu_set_flow_ctl(state_, ctl_)         ((state_)->flow.ctl = (ctl_))
#define cpu_get_flow_ctl(state_)               ((state_)->flow.ctl)

#define cpu_increase_pc(state_, inc_)          cpu_set_pc(state_, cpu_get_pc(state_) + (inc_))
#define cpu_commit_pc(state_)                  cpu_set_pc(state_, cpu_get_flow_pc(state_))

#define cpu_set_gpr(state_, reg_, val_) \
    do { \
        assert((reg_) < NUM_GPRS); \
        if ((reg_) != GPR_ZERO) (state_)->gp_regs[(reg_)] = (val_); \
    } while (0)
#define cpu_get_gpr(state_, reg_) \
    (assert((reg_) < NUM_GPRS), \
       ((reg_) == GPR_ZERO) ? 0 : (state_)->gp_regs[(reg_)])

#define cpu_set_fpr(state_, reg_, val_)         (assert((reg_) < NUM_FPRS), (state_)->fp_regs[(reg_)] = (val_))
#define cpu_get_fpr(state_, reg_)               (assert((reg_) < NUM_FPRS), (state_)->fp_regs[(reg_)])

#define cpu_set_fpr_q(state_, reg_, val_)       (assert((reg_) < NUM_FPRS), (state_)->fp_regs[(reg_)].q = (val_))
#define cpu_get_fpr_q(state_, reg_)             (assert((reg_) < NUM_FPRS), (state_)->fp_regs[(reg_)].q)

#define cpu_set_fpr_w(state_, reg_, val_)       (assert((reg_) < NUM_FPRS), (state_)->fp_regs[(reg_)].w = (val_))
#define cpu_get_fpr_w(state_, reg_)             (assert((reg_) < NUM_FPRS), (state_)->fp_regs[(reg_)].w)

#define cpu_set_fpr_d(state_, reg_, val_)       (assert((reg_) < NUM_FPRS), (state_)->fp_regs[(reg_)].d = (val_))
#define cpu_get_fpr_d(state_, reg_)             (assert((reg_) < NUM_FPRS), (state_)->fp_regs[(reg_)].d)

#define cpu_set_fpr_s(state_, reg_, val_)       (assert((reg_) < NUM_FPRS), (state_)->fp_regs[(reg_)].s = (val_))
#define cpu_get_fpr_s(state_, reg_)             (assert((reg_) < NUM_FPRS), (state_)->fp_regs[(reg_)].s)

#endif // CPU_H
