#include "decoder.h"
#include "cpu.h"
#include "mmu.h"

#include <stdlib.h>
#include <math.h>

static inline u64 _mulhu(u64 a, u64 b)
{
    (void) a;
    (void) b;
    return 0;
}

static inline i64 _mulh(i64 a, i64 b)
{
    (void) a;
    (void) b;
    return 0;
}

static inline i64 _mulhsu(i64 a, i64 b)
{
    (void) a;
    (void) b;
    return 0;
}

static inline i64 _div(i64 a, i64 b)
{
    if (b == 0)
        return -1;
    else if (a == INT64_MIN && b == -1)
        return INT64_MIN;
    else
        return a / b;
}

static inline i64 _rem(i64 a, i64 b)
{
    if (b == 0)
        return a;
    else if (a == INT64_MIN && b == -1)
        return 0;
    else
        return a % b;
}

static inline u64 _fsgnj(u64 a, u64 b, u64 size, bool neg, bool xor)
{
    (void) a;
    (void) b;
    (void) size;
    (void) neg;
    (void) xor;
    return 0;
}

static inline u16 _f_classify(f64 a, u64 size)
{
    (void) a;
    (void) size;
    return 0;
}

#define GEN(name, tag, a1, a2, a3, a4) GEN_##tag(name, a1, a2, a3, a4)

#define GEN_EMPTY(name, a1, a2, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    (void) cpup; \
    (void) instp; \
}

#define GEN_LOAD(name, type, a2, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    GuestVAddr addr = cpup->gp_regs[instp->rs1] + (i64) instp->imm; \
    cpup->gp_regs[instp->rd] = *(type *) TO_HOST(addr); \
}

#define GEN_OP_IMM(name, expr, a2, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    u64 rs1 = cpup->gp_regs[instp->rs1]; \
    i64 imm = (i64) instp->imm; \
    cpup->gp_regs[instp->rd] = (expr); \
}

#define GEN_AUIPC(name, a1, a2, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    u64 val = cpup->pc + (i64) instp->imm; \
    cpup->gp_regs[instp->rd] = val; \
}

#define GEN_STORE(name, type, a2, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    GuestVAddr src  = cpup->gp_regs[instp->rs2]; \
    GuestVAddr dest = cpup->gp_regs[instp->rs1] + (i64) instp->imm; \
    *(type *) TO_HOST(dest) = (type) src; \
}

#define GEN_OP_REG(name, expr, a2, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    u64 rs1 = cpup->gp_regs[instp->rs1]; \
    u64 rs2 = cpup->gp_regs[instp->rs2]; \
    cpup->gp_regs[instp->rd] = (expr); \
}

#define GEN_LUI(name, a1, a2, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    cpup->gp_regs[instp->rd] = (i64) instp->imm; \
}

#define GEN_BRANCH(name, expr, a2, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    u64 rs1 = cpup->gp_regs[instp->rs1]; \
    u64 rs2 = cpup->gp_regs[instp->rs2]; \
    GuestVAddr target_addr = cpup->pc + (i64) instp->imm; \
    if (expr) { \
        cpup->reenter_pc = cpup->pc = target_addr; \
        cpup->brkcode = JUMP; \
        instp->brk = true; \
    } \
}

#define GEN_JUMP(name, addr, a2, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    u64 rs1 = cpup->gp_regs[instp->rs1]; \
    i64 imm = (i64) instp->imm; \
    u64 pc = cpup->pc; \
    cpup->gp_regs[instp->rd] = pc + (instp->rvc ? 2 : 4); \
    cpup->reenter_pc = (addr); \
    cpup->brkcode = JUMP; \
    (void) rs1; \
}

#define GEN_ECALL(name, a1, a2, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    (void) instp; \
    cpup->reenter_pc = cpup->pc + 4; \
    cpup->brkcode = ECALL; \
}

#define GEN_CSR(name, a1, a2, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    u16 csr = (u16) instp->imm; \
    switch (csr) { \
    case FFLAGS: \
    case FRM: \
    case FCSR: \
        break; \
    default: \
        fatal("unsupported csr"); \
    } \
    cpup->gp_regs[instp->rd] = 0; \
}

#define GEN_FP_LOAD(name, type, a2, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    GuestVAddr addr = cpup->gp_regs[instp->rs1] + (i64) instp->imm; \
    u64 val = sizeof(type) == 8 ? \
        *(u64 *) TO_HOST(addr) : \
        *(u32 *) TO_HOST(addr) | (-1ULL << 32); \
    cpup->fp_regs[instp->rd].q = val; \
}

#define GEN_FP_STORE(name, type, a2, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    GuestVAddr src  = cpup->fp_regs[instp->rs2].q; \
    GuestVAddr dest = cpup->gp_regs[instp->rs1] + (i64) instp->imm; \
    *(type *) TO_HOST(dest) = (type) src; \
}

#define GEN_FP_FMA4(name, view, type, expr, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    type rs1 = cpup->fp_regs[instp->rs1].view; \
    type rs2 = cpup->fp_regs[instp->rs2].view; \
    type rs3 = cpup->fp_regs[instp->rs3].view; \
    cpup->fp_regs[instp->rd].view = (type) (expr); \
}

#define GEN_FP_BINOP(name, view, type, expr, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    type rs1 = cpup->fp_regs[instp->rs1].view; \
    type rs2 = cpup->fp_regs[instp->rs2].view; \
    cpup->fp_regs[instp->rd].view = (type) (expr); \
    (void) rs2; \
}

#define GEN_FP_SGNJ(name, view, type, neg, xor) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    type rs1 = cpup->fp_regs[instp->rs1].view; \
    type rs2 = cpup->fp_regs[instp->rs2].view; \
    cpup->fp_regs[instp->rd].q = _fsgnj(rs1, rs2, sizeof(type), neg, xor); \
}

#define GEN_FP_XFER_F2I(name, view, type, expr, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    type src = cpup->fp_regs[instp->rs1].view; \
    cpup->gp_regs[instp->rd] = (expr); \
}

#define GEN_FP_XFER_I2F(name, view, expr, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) { \
    u64 src = cpup->gp_regs[instp->rs1]; \
    cpup->fp_regs[instp->rd].view = (expr); \
}

#define GEN_FP_XFER_F2F(name, sview, dview, type, expr) \
static void func_##name(CPU *cpup, Inst *instp) { \
    type src = cpup->fp_regs[instp->rs1].sview; \
    cpup->fp_regs[instp->rd].dview = (expr); \
}

#define GEN_FP_CMP(name, view, type, expr, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    type rs1 = cpup->fp_regs[instp->rs1].view; \
    type rs2 = cpup->fp_regs[instp->rs2].view; \
    cpup->gp_regs[instp->rd] = (expr); \
}

#define GEN_FP_CLASS(name, view, type, a3, a4) \
static void func_##name(CPU *cpup, Inst *instp) \
{ \
    cpup->gp_regs[instp->rd] = _f_classify(cpup->fp_regs[instp->rs1].view, sizeof(type)); \
}

INSTRUCTIONS(GEN)

#undef GEN // Generate instruction handler

#define GEN(name, tag, a1, a2, a3, a4) [IK_##name] = func_##name,
typedef void (Func)(CPU *, Inst *);
static Func *funcs[] = { INSTRUCTIONS(GEN) };
#undef GEN // Generate dispatch table

void exec_block_interp(CPU *cpup)
{
    Inst inst = {0};
    while (1) {
        // Fetch
        u32 data = *(u32 *) TO_HOST(cpup->pc);

        // Decode
        inst_decode(&inst, data);

        // Execute
        funcs[inst.kind](cpup, &inst);
        cpup->gp_regs[RI_ZERO] = 0; // Writes to x0 are ignored; x0 is hardwired to 0

        // Step
        if (inst.brk)
            break;
        else
            cpup->pc += inst.rvc ? 2 : 4;
    }
}
