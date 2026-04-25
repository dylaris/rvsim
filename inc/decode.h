#ifndef DECODE_H
#define DECODE_H

#include "common.h"
#include "template.h"

#define X(name, tag, a1, a2, a3, a4) instr_##name,
typedef enum { INSTRUCTION_LIST(X) NUM_INSTRS, } InstrKind;
#undef X

typedef struct {
    i8 rd;
    i8 rs1;
    i8 rs2;
    i8 rs3;
    i32 imm;
    InstrKind kind;
    bool rvc; // Is RISCV compression extension?
    bool cfc; // Is control flow changed?
    u64 curr_pc;
    u64 next_pc;
} Instr;

void decode_instr(u64 pc, Instr *out);
const char *instr_to_string(const Instr *instr);

#endif // DECODE_H
