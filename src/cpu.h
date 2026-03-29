#ifndef CPU_H
#define CPU_H

#include "utils.h"

typedef enum {
    RI_ZERO = 0, RI_RA, RI_SP, RI_GP, RI_TP,
    RI_T0, RI_T1, RI_T2,
    RI_S0, RI_S1,
    RI_A0, RI_A1, RI_A2, RI_A3, RI_A4, RI_A5, RI_A6, RI_A7,
    RI_S2, RI_S3, RI_S4, RI_S5, RI_S6, RI_S7, RI_S8, RI_S9, RI_S10, RI_S11,
    RI_T3, RI_T4, RI_T5, RI_T6,
    num_gp_regs,
} GPRegIndex;

typedef enum {
    RI_FT0 = 0, RI_FT1, RI_FT2, RI_FT3, RI_FT4, RI_FT5, RI_FT6, RI_FT7,
    RI_FS0, RI_FS1,
    RI_FA0, RI_FA1, RI_FA2, RI_FA3, RI_FA4, RI_FA5, RI_FA6, RI_FA7,
    RI_FS2, RI_FS3, RI_FS4, RI_FS5, RI_FS6, RI_FS7, RI_FS8, RI_FS9, RI_FS10, RI_FS11,
    RI_FT8, RI_FT9, RI_FT10, RI_FT11,
    num_fp_regs,
} FPRegIndex;

typedef enum {
    NONE = 0,
    JUMP,
    ECALL
} BreakCode;

typedef u64 GPReg;
typedef union {
    u64 q;
    f64 d;
    u32 w;
    f32 s;
} FPReg;

typedef struct {
    BreakCode brkcode;  // Reason for breaking out of the current block
    GPReg gp_regs[num_gp_regs];    // General purpose registers
    FPReg fp_regs[num_fp_regs];    // Float Point registers
    u64 pc;
    u64 reenter_pc;
} CPU;

#endif // CPU_H
