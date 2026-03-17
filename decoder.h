#ifndef DECODER_H
#define DECODER_H

#include "utils.h"

#include <stdbool.h>

typedef enum {
    IK_ADDI,
    num_insts,
} InstKind;

typedef struct {
    u8 rd;
    u8 rs1;
    u8 rs2;
    i32 imm;
    InstKind kind;
    bool rvc;   // Is RISCV compression extension?
    bool brk;   // Break out of current block (control flow changed)
} Instruction;

typedef enum {
    RI_ZERO, RI_RA, RI_SP, RI_GP, RI_TP,
    RI_T0, RI_T1, RI_T2,
    RI_S0, RI_S1,
    RI_A0, RI_A1, RI_A2, RI_A3, RI_A4, RI_A5, RI_A6, RI_A7,
    RI_S2, RI_S3, RI_S4, RI_S5, RI_S6, RI_S7, RI_S8, RI_S9, RI_S10, RI_S11,
    RI_T3, RI_T4, RI_T5, RI_T6,
    num_regs,
} RegIndex;

void inst_decode(Instruction *instp, u32 data);

#endif // DECODER_H
