#ifndef DECODE_H
#define DECODE_H

#include "common.h"

#include "instr_template.h"

#define X(name, tag, a1, a2, a3, a4) INSTR_##name,
typedef enum { INSTRUCTION_LIST(X) NUM_INSTRS, } InstrKind;
#undef X

// Dynamic runtime instruction
typedef struct {
    i8 rd;
    i8 rs1;
    i8 rs2;
    i8 rs3;
    i32 imm;
    InstrKind kind;
    bool rvc; // Is RISCV compression extension?
    bool cfc; // Is control flow changed?
} Instr;

// Static instruction metadata
typedef struct InstrInfo InstrInfo;
struct InstrInfo {
    const char *name;
    InstrKind kind;

    u16 opcode;      // [6:0]
    u16 funct2;      // [26:25]
    u16 funct3;      // [14:12]
    u16 funct5high;  // [31:27]
    u16 funct5low;   // [24:20]
    u16 funct6;      // [31:26]
    u16 funct7;      // [31:25]
    u16 funct12;     // [31:20]

    u16 copcode;     // [1:0]
    u16 cfunct1;     // [12]
    u16 cfunct2high; // [11:10]
    u16 cfunct2low;  // [6:5]
    u16 cfunct3;     // [15:13]
    u16 cfunct5high; // [11:7]
    u16 cfunct5low;  // [6:2]

    Instr (*decode)(const InstrInfo *, u32);
};

typedef enum {
    CSR_FFLAGS = 0x001,
    CSR_FRM    = 0x002,
    CSR_FCSR   = 0x003,
} CSR;

bool decode_instr(u32 raw, Instr *out);
bool decode_instr_info(u32 raw, InstrInfo *out);
const char *instr_to_string(InstrKind kind);

#endif // DECODE_H
