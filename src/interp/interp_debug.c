#include "interp.h"
#include "decode.h"

#define GEN_EMPTY(name, a1, a2, a3, a4) \
SIGNATURE(name) { }

#define GEN_LOAD(name, type, a2, a3, a4) \
SIGNATURE(name) \
{ \
    GuestVAddr addr = cpu_get_gpr(state, instr->rs1) + (i64) instr->imm; \
    cpu_set_gpr(state, instr->rd, mem_read_##type(addr)); \
}

#define GEN_OP_IMM(name, expr, a2, a3, a4) \
SIGNATURE(name) \
{ \
    u64 rs1 = cpu_get_gpr(state, instr->rs1); \
    i64 imm = (i64) instr->imm; \
    cpu_set_gpr(state, instr->rd, (expr)); \
}

#define GEN_AUIPC(name, a1, a2, a3, a4) \
SIGNATURE(name) \
{ \
    u64 val = instr->curr_pc + (i64) instr->imm; \
    cpu_set_gpr(state, instr->rd, val); \
}

#define GEN_STORE(name, type, a2, a3, a4) \
SIGNATURE(name) \
{ \
    u64 val  = cpu_get_gpr(state, instr->rs2); \
    GuestVAddr addr = cpu_get_gpr(state, instr->rs1) + (i64) instr->imm; \
    mem_write_##type(addr, (type) val); \
}

#define GEN_OP_REG(name, expr, a2, a3, a4) \
SIGNATURE(name) \
{ \
    u64 rs1 = cpu_get_gpr(state, instr->rs1); \
    u64 rs2 = cpu_get_gpr(state, instr->rs2); \
    cpu_set_gpr(state, instr->rd, (expr)); \
}

#define GEN_OP_HMUL(name, expr, a2, a3, a4) \
SIGNATURE(name) \
{ \
    u64 rs1 = cpu_get_gpr(state, instr->rs1); \
    u64 rs2 = cpu_get_gpr(state, instr->rs2); \
    cpu_set_gpr(state, instr->rd, (expr)); \
}

#define GEN_LUI(name, a1, a2, a3, a4) \
SIGNATURE(name) \
{ \
    cpu_set_gpr(state, instr->rd, (i64) instr->imm); \
}

#define GEN_BRANCH(name, expr, a2, a3, a4) \
SIGNATURE(name) \
{ \
    u64 rs1 = cpu_get_gpr(state, instr->rs1); \
    u64 rs2 = cpu_get_gpr(state, instr->rs2); \
    GuestVAddr addr = instr->curr_pc + (i64) instr->imm; \
    instr->cfc = false; \
    if (expr) { \
        instr->cfc = true; \
        cpu_set_flow(state, FLOW_BRANCH_TAKEN); \
        cpu_set_pc(state, addr); \
    } else { \
        cpu_set_flow(state, FLOW_BRANCH_NOT_TAKEN); \
    } \
}

#define GEN_JUMP(name, addr, a2, a3, a4) \
SIGNATURE(name) \
{ \
    u64 rs1 = cpu_get_gpr(state, instr->rs1); \
    i64 imm = (i64) instr->imm; \
    u64 pc = instr->curr_pc; \
    cpu_set_gpr(state, instr->rd, instr->next_pc); \
    cpu_set_flow(state, instr->kind == instr_jal ? FLOW_DIRECT_JUMP : FLOW_INDIRECT_JUMP); \
    cpu_set_pc(state, addr); \
}

#define GEN_ECALL(name, a1, a2, a3, a4) \
SIGNATURE(name) \
{ \
    cpu_set_flow(state, FLOW_ECALL); \
    cpu_set_pc(state, instr->next_pc); \
}

#define GEN_CSR(name, a1, a2, a3, a4) \
SIGNATURE(name) \
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
SIGNATURE(name) \
{ \
    GuestVAddr addr = cpu_get_gpr(state, instr->rs1) + (i64) instr->imm; \
    u64 val = sizeof(type) == 8 ? \
        mem_read_u64(addr) : \
        mem_read_u32(addr) | (-1ULL << 32); \
    cpu_set_fpr_q(state, instr->rd, val); \
}

#define GEN_FP_STORE(name, type, a2, a3, a4) \
SIGNATURE(name) \
{ \
    u64 src = cpu_get_fpr_q(state, instr->rs2); \
    GuestVAddr addr = cpu_get_gpr(state, instr->rs1) + (i64) instr->imm; \
    mem_write_##type(addr, (type) src); \
}

#define GEN_FP_FMA4(name, view, type, expr, a4) \
SIGNATURE(name) \
{ \
    type rs1 = cpu_get_fpr_##view(state, instr->rs1); \
    type rs2 = cpu_get_fpr_##view(state, instr->rs2); \
    type rs3 = cpu_get_fpr_##view(state, instr->rs3); \
    cpu_set_fpr_##view(state, instr->rd, (type) (expr)); \
}

#define GEN_FP_BINOP(name, view, type, expr, a4) \
SIGNATURE(name) \
{ \
    type rs1 = cpu_get_fpr_##view(state, instr->rs1); \
    type rs2 = cpu_get_fpr_##view(state, instr->rs2); \
    cpu_set_fpr_##view(state, instr->rd, (type) (expr)); \
}

#define GEN_FP_SGNJ(name, view, type, neg, xor) \
SIGNATURE(name) \
{ \
    type rs1 = cpu_get_fpr_##view(state, instr->rs1); \
    type rs2 = cpu_get_fpr_##view(state, instr->rs2); \
    cpu_set_fpr_q(state, instr->rd, help_fsgnj(rs1, rs2, sizeof(type), neg, xor)); \
}

#define GEN_FP_XFER_F2I(name, view, type, expr, a4) \
SIGNATURE(name) \
{ \
    type src = cpu_get_fpr_##view(state, instr->rs1); \
    cpu_set_gpr(state, instr->rd, (expr)); \
}

#define GEN_FP_XFER_I2F(name, view, expr, a3, a4) \
SIGNATURE(name) \
{ \
    u64 src = cpu_get_gpr(state, instr->rs1); \
    cpu_set_fpr_##view(state, instr->rd, (expr)); \
}

#define GEN_FP_XFER_F2F(name, sview, dview, type, expr) \
SIGNATURE(name) \
{ \
    type src = cpu_get_fpr_##sview(state, instr->rs1); \
    cpu_set_fpr_##dview(state, instr->rd, (expr)); \
}

#define GEN_FP_CMP(name, view, type, expr, a4) \
SIGNATURE(name) \
{ \
    type rs1 = cpu_get_fpr_##view(state, instr->rs1); \
    type rs2 = cpu_get_fpr_##view(state, instr->rs2); \
    cpu_set_gpr(state, instr->rd, (expr)); \
}

#define GEN_FP_CLASS(name, view, type, a3, a4) \
SIGNATURE(name) \
{ \
    cpu_set_gpr(state, instr->rd, help_f_classify(state->fp_regs[instr->rs1].view, sizeof(type))); \
}

#define SIGNATURE(name) static void interp__##name(CPUState *state, Instr *instr)
#define GEN(name, tag, a1, a2, a3, a4) GEN_##tag(name, a1, a2, a3, a4)

INSTRUCTION_LIST(GEN)

#define X(name, tag, a1, a2, a3, a4) [instr_##name] = interp__##name,
typedef void (*InstrFunc)(CPUState *, Instr *);
static InstrFunc dispatch_table_debug[] = { INSTRUCTION_LIST(X) };
#undef X

void interp(Machine *machine)
{
    CPUState *state = &machine->state;
    cpu_reset_flow(state);
    Instr instr = {0};
    u64 pc = cpu_get_pc(state);
    decode_instr(pc, &instr);
    cpu_set_pc(state, instr.next_pc);
    InstrFunc func = dispatch_table_debug[instr.kind];
    func(state, &instr);
    if (IS_TRAP(cpu_get_flow(state))) machine_trap(machine);
}

#undef SIGNATURE
#undef GEN
#undef GEN_EMPTY
#undef GEN_LOAD
#undef GEN_OP_IMM
#undef GEN_AUIPC
#undef GEN_STORE
#undef GEN_OP_REG
#undef GEN_OP_HMUL
#undef GEN_LUI
#undef GEN_BRANCH
#undef GEN_JUMP
#undef GEN_ECALL
#undef GEN_CSR
#undef GEN_FP_LOAD
#undef GEN_FP_STORE
#undef GEN_FP_FMA4
#undef GEN_FP_BINOP
#undef GEN_FP_SGNJ
#undef GEN_FP_XFER_F2I
#undef GEN_FP_XFER_I2F
#undef GEN_FP_XFER_F2F
#undef GEN_FP_CMP
#undef GEN_FP_CLASS

