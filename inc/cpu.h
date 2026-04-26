#ifndef CPU_H
#define CPU_H

#include "reg.h"
#include "flow.h"

typedef struct {
    u64 pc;
    Flow flow;
    GPR gp_regs[NUM_GPRS];
    FPR fp_regs[NUM_FPRS];
} CPUState;

#define cpu_set_pc(state_, pc_)                ((state_)->pc = (pc_))
#define cpu_get_pc(state_)                     ((state_)->pc)
#define cpu_increase_pc(state_, inc_)          ((state_)->pc += (inc_))

#define cpu_reset_flow(state_)                 ((state_)->flow = FLOW_NONE)
#define cpu_set_flow(state_, flow_)            ((state_)->flow = (flow_))
#define cpu_get_flow(state_)                   ((state_)->flow)

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
