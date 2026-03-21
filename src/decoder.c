#include "decoder.h"

#define QUADRANT(data) (((data) >>  0) & 0x3 )
#define OPCODE(data)   (((data) >>  0) & 0x7f)
#define RD(data)       (((data) >>  7) & 0x1f)
#define RS1(data)      (((data) >> 15) & 0x1f)
#define RS2(data)      (((data) >> 20) & 0x1f)
#define RS3(data)      (((data) >> 27) & 0x1f)
#define FUNCT2(data)   (((data) >> 25) & 0x3 )
#define FUNCT3(data)   (((data) >> 12) & 0x7 )
#define FUNCT7(data)   (((data) >> 25) & 0x7f)

static inline i32 inst__sign_extend(i32 x, u32 n)
{
    return (i32) ((x << (32 - n)) >> (32 - n));
}

static inline Instruction inst__decode_rtype(InstKind kind, u32 data)
{
    return (Instruction) {
        .kind = kind,
        .rd   = RD(data),
        .rs1  = RS1(data),
        .rs2  = RS2(data),
    };
}

static inline Instruction inst__decode_itype(InstKind kind, u32 data)
{
    return (Instruction) {
        .kind = kind,
        .rd   = RD(data),
        .rs1  = RS1(data),
        .imm  = (i32) ((data >> 20) & 0xfff)
    };
}

static inline Instruction inst__decode_stype(InstKind kind, u32 data)
{
    u32 imm4_0  = (u32) ((data >>  7) & 0x1f);
    u32 imm11_5 = (u32) ((data >> 25) & 0x7f);
    i32 imm     = (i32) ((imm11_5 << 5) | imm4_0);
    return (Instruction) {
        .kind = kind,
        .rd   = RD(data),
        .rs1  = RS1(data),
        .imm  = inst__sign_extend(imm, 12)
    };
}

static inline Instruction inst__decode_btype(InstKind kind, u32 data)
{
    u32 imm11   = (u32) ((data >>  7) & 0x1);
    u32 imm4_1  = (u32) ((data >>  8) & 0xf);
    u32 imm10_5 = (u32) ((data >> 25) & 0x3f);
    u32 imm12   = (u32) ((data >> 31) & 0x1);
    i32 imm     = (i32) ((imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1 << 1));
    return (Instruction) {
        .kind = kind,
        .rs1  = RS1(data),
        .rs2  = RS2(data),
        .imm  = inst__sign_extend(imm, 13)
    };
}

static inline Instruction inst__decode_utype(InstKind kind, u32 data)
{
    return (Instruction) {
        .kind = kind,
        .rd   = RD(data),
        .imm  = (i32) (data & 0xfffff000)
    };
}

static inline Instruction inst__decode_jtype(InstKind kind, u32 data)
{
    u32 imm19_12 = (u32) ((data >> 12) & 0xff);
    u32 imm11    = (u32) ((data >> 20) & 0x1);
    u32 imm10_1  = (u32) ((data >> 21) & 0x3ff);
    u32 imm20    = (u32) ((data >> 31) & 0x1);
    i32 imm      = (i32) ((imm20 << 20) | (imm19_12 << 12) | (imm11 << 11) | (imm10_1 << 1));
    return (Instruction) {
        .kind = kind,
        .rd   = RD(data),
        .imm  = inst__sign_extend(imm, 21)
    };
}

#define INST_IGNORE_FIELD 0xff
typedef struct {
    const char *name;
    InstKind kind;
    u8 opcode;
    u8 funct3;
    u8 funct7;
    Instruction (*decode)(InstKind, u32);
} InstDef;

static InstDef inst_table[] = {
    { "add", IK_ADD, 0x33, 0x0, 0x00, inst__decode_rtype },
    { NULL, 0, 0, 0, 0, NULL },
};

void inst_decode(Instruction *instp, u32 data)
{
    u8 opcode = OPCODE(data);
    u8 funct3 = FUNCT3(data);
    u8 funct7 = FUNCT7(data);

    for (InstDef *defp = inst_table; defp->name; defp++) {
        if (defp->opcode == opcode && defp->funct3 == funct3 &&
           (defp->funct7 == funct7 || defp->funct7 == INST_IGNORE_FIELD)) {
            *instp = defp->decode(defp->kind, data);
            return;
        }
    }
    fatalf("failed to decode: %d(opcode), %d(funct3), %d(funct7)", opcode, funct3, funct7);
}

void inst_print(Instruction inst)
{
    switch (inst.kind) {
    case IK_ADD:
        printf("name: add\nrd: %d\nrs1: %d\nrs2: %d\n", inst.rd, inst.rs1, inst.rs2);
        break;
    default:
        fatal("unimplemented");
    }
}
