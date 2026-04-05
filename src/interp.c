#include "interp.h"
#include "decode.h"

#define GEN(name, tag, a1, a2, a3, a4) GEN_##tag(name, a1, a2, a3, a4)

#define GEN_EMPTY(name, a1, a2, a3, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    (void) state; \
    (void) instr; \
}

#define GEN_LOAD(name, type, a2, a3, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    GuestVAddr addr = cpu_get_gpr(state, instr->rs1) + (i64) instr->imm; \
    cpu_set_gpr(state, instr->rd, mem_read_##type(addr)); \
}

#define GEN_OP_IMM(name, expr, a2, a3, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    u64 rs1 = cpu_get_gpr(state, instr->rs1); \
    i64 imm = (i64) instr->imm; \
    cpu_set_gpr(state, instr->rd, (expr)); \
}

#define GEN_AUIPC(name, a1, a2, a3, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    u64 val = state->pc + (i64) instr->imm; \
    cpu_set_gpr(state, instr->rd, val); \
}

#define GEN_STORE(name, type, a2, a3, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    u64 val  = cpu_get_gpr(state, instr->rs2); \
    GuestVAddr addr = cpu_get_gpr(state, instr->rs1) + (i64) instr->imm; \
    mem_write_##type(addr, (type) val); \
}

#define GEN_OP_REG(name, expr, a2, a3, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    u64 rs1 = cpu_get_gpr(state, instr->rs1); \
    u64 rs2 = cpu_get_gpr(state, instr->rs2); \
    cpu_set_gpr(state, instr->rd, (expr)); \
}

#define GEN_LUI(name, a1, a2, a3, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    cpu_set_gpr(state, instr->rd, (i64) instr->imm); \
}

#define GEN_BRANCH(name, expr, a2, a3, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    u64 rs1 = cpu_get_gpr(state, instr->rs1); \
    u64 rs2 = cpu_get_gpr(state, instr->rs2); \
    GuestVAddr addr = state->pc + (i64) instr->imm; \
    if (expr) { \
        instr->cfc = true; \
        state->flow.ctl = FLOW_BRANCH; \
        state->flow.pc = addr; \
    } \
}

#define GEN_JUMP(name, addr, a2, a3, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    u64 rs1 = cpu_get_gpr(state, instr->rs1); \
    i64 imm = (i64) instr->imm; \
    u64 pc = state->pc; \
    cpu_set_gpr(state, instr->rd, pc + (instr->rvc ? 2 : 4)); \
    instr->cfc = true; \
    state->flow.ctl = FLOW_JUMP; \
    state->flow.pc = (addr); \
    (void) rs1; \
}

#define GEN_ECALL(name, a1, a2, a3, a4)
static void interp__ecall(CPUState *state, Instr *instr)
{
#ifdef TEST_TVM
    if (cpu_get_gpr(state, GPR_A7) == SYS_exit) {
        u64 ret = cpu_get_gpr(state, GPR_A0);
        if (ret == 0)
            printf("Test PASS\n");
        else
            printf("Test #%lu FAIL\n", ret / 2);
    }
#endif
    instr->cfc = true;
    state->flow.ctl = FLOW_ECALL;
    state->flow.pc = state->pc + 4;
}

#define GEN_CSR(name, a1, a2, a3, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    u16 csr = (u16) instr->imm; \
    switch (csr) { \
    case CSR_FFLAGS: \
    case CSR_FRM: \
    case CSR_FCSR: \
        break; \
    default: \
        fatal("unsupported csr"); \
    } \
    cpu_set_gpr(state, instr->rd, 0); \
}

#define GEN_FP_LOAD(name, type, a2, a3, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    GuestVAddr addr = cpu_get_gpr(state, instr->rs1) + (i64) instr->imm; \
    u64 val = sizeof(type) == 8 ? \
        mem_read_u64(addr) : \
        mem_read_u32(addr) | (-1ULL << 32); \
    cpu_set_fpr_q(state, instr->rd, val); \
}

#define GEN_FP_STORE(name, type, a2, a3, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    u64 src = cpu_get_fpr_q(state, instr->rs2); \
    GuestVAddr addr = cpu_get_gpr(state, instr->rs1) + (i64) instr->imm; \
    mem_write_##type(addr, (type) src); \
}

#define GEN_FP_FMA4(name, view, type, expr, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    type rs1 = cpu_get_fpr_##view(state, instr->rs1); \
    type rs2 = cpu_get_fpr_##view(state, instr->rs2); \
    type rs3 = cpu_get_fpr_##view(state, instr->rs3); \
    cpu_set_fpr_##view(state, instr->rd, (type) (expr)); \
}

#define GEN_FP_BINOP(name, view, type, expr, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    type rs1 = cpu_get_fpr_##view(state, instr->rs1); \
    type rs2 = cpu_get_fpr_##view(state, instr->rs2); \
    state->fp_regs[instr->rd].view = (type) (expr); \
    cpu_set_fpr_##view(state, instr->rd, (type) (expr)); \
    (void) rs2; \
}

#define GEN_FP_SGNJ(name, view, type, neg, xor) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    type rs1 = cpu_get_fpr_##view(state, instr->rs1); \
    type rs2 = cpu_get_fpr_##view(state, instr->rs2); \
    cpu_set_fpr_q(state, instr->rd, help_fsgnj(rs1, rs2, sizeof(type), neg, xor)); \
}

#define GEN_FP_XFER_F2I(name, view, type, expr, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    type src = cpu_get_fpr_##view(state, instr->rs1); \
    cpu_set_gpr(state, instr->rd, (expr)); \
}

#define GEN_FP_XFER_I2F(name, view, expr, a3, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    u64 src = cpu_get_gpr(state, instr->rs1); \
    cpu_set_fpr_##view(state, instr->rd, (expr)); \
}

#define GEN_FP_XFER_F2F(name, sview, dview, type, expr) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    type src = cpu_get_fpr_##sview(state, instr->rs1); \
    cpu_set_fpr_##dview(state, instr->rd, (expr)); \
}

#define GEN_FP_CMP(name, view, type, expr, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    type rs1 = cpu_get_fpr_##view(state, instr->rs1); \
    type rs2 = cpu_get_fpr_##view(state, instr->rs2); \
    cpu_set_gpr(state, instr->rd, (expr)); \
}

#define GEN_FP_CLASS(name, view, type, a3, a4) \
static void interp__##name(CPUState *state, Instr *instr) \
{ \
    cpu_set_gpr(state, instr->rd, help_f_classify(state->fp_regs[instr->rs1].view, sizeof(type))); \
}

INSTRUCTION_LIST(GEN)

#undef GEN // Generate instruction handler

#define X(name, tag, a1, a2, a3, a4) [INSTR_##name] = interp__##name,
typedef void (*InterpFunc)(CPUState *, Instr *);
static InterpFunc funcs[] = { INSTRUCTION_LIST(X) };
#undef X // Generate dispatch table

void interp_block(CPUState *state)
{
    Instr instr = {0};

    while (1) {
        u32 raw = mem_read_u32(state->pc);

        if (!decode_instr(raw, &instr)) {
            state->flow.ctl = FLOW_ILLEGAL_INSTR;
            return;
        }

        funcs[instr.kind](state, &instr);

        if (instr.cfc)
            break;
        else
            state->pc += instr.rvc ? 2 : 4;
    }
}
