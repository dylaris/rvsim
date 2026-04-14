#include "interp.h"
#include "decode.h"

#define SIGNATURE(name) static void interp__##name(CPUState *state, Instr *instr)

#define GEN(name, tag, a1, a2, a3, a4) GEN_##tag(name, a1, a2, a3, a4)

#define GEN_EMPTY(name, a1, a2, a3, a4) \
SIGNATURE(name) \
{ \
    (void) state; \
    (void) instr; \
}

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
    u64 val = cpu_get_pc(state) + (i64) instr->imm; \
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
    GuestVAddr addr = cpu_get_pc(state) + (i64) instr->imm; \
    if (expr) { \
        cpu_set_flow_ctl(state, FLOW_BRANCH); \
        cpu_set_flow_pc(state, addr); \
    } \
}

#define GEN_JUMP(name, addr, a2, a3, a4) \
SIGNATURE(name) \
{ \
    u64 rs1 = cpu_get_gpr(state, instr->rs1); \
    i64 imm = (i64) instr->imm; \
    u64 pc = cpu_get_pc(state); \
    cpu_set_gpr(state, instr->rd, pc + (instr->rvc ? 2 : 4)); \
    cpu_set_flow_ctl(state, FLOW_JUMP); \
    cpu_set_flow_pc(state, addr); \
    (void) rs1; \
}

#define GEN_ECALL(name, a1, a2, a3, a4) \
SIGNATURE(name) \
{ \
    (void) instr; \
    GuestVAddr addr = cpu_get_pc(state) + 4; \
    cpu_set_flow_ctl(state, FLOW_ECALL); \
    cpu_set_flow_pc(state, addr); \
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
    (void) rs2; \
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

INSTRUCTION_LIST(GEN)

#undef SIGNATURE
#undef GEN
#undef GEN_EMPTY
#undef GEN_LOAD
#undef GEN_OP_IMM
#undef GEN_AUIPC
#undef GEN_STORE
#undef GEN_OP_REG
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

#define X(name, tag, a1, a2, a3, a4) [instr_##name] = interp__##name,
typedef void (*InstrFunc)(CPUState *, Instr *);
static InstrFunc dispatch_table[] = { INSTRUCTION_LIST(X) };
#undef X

#ifdef DEBUG
void interp_single(Machine *machine)
{
    CPUState *state = &machine->state;
#else
void interp_single(CPUState *state)
{
#endif
    Instr instr = {0};

#ifdef DEBUG
    if (machine->single_step)
        cpu_set_flow_ctl(state, FLOW_HALT);
#endif

    u32 raw = mem_read_u32(cpu_get_pc(state));

    if (unlikely(!decode_instr(raw, &instr))) {
        cpu_set_flow_ctl(state, FLOW_ILLEGAL_INSTR);
        return;
    }

    cpu_increase_flow_pc(state, instr.rvc ? 2 : 4);

    InstrFunc func = dispatch_table[instr.kind];
    func(state, &instr);

    if (instr.cfc)
        return;
    cpu_commit_pc(state);
}

Record record = {0};

#ifdef DEBUG
void interp_block(Machine *machine)
{
    CPUState *state = &machine->state;
#else
void interp_block(CPUState *state)
{
#endif
    Instr instr = {0};

    while (true) {
        u64 pc = cpu_get_pc(state);

        u32 raw = mem_read_u32(pc);

        if (unlikely(!decode_instr(raw, &instr))) {
            cpu_set_flow_ctl(state, FLOW_ILLEGAL_INSTR);
            break;
        }
        if (instr.kind == instr_ebreak) {
            printf("[%lx] %s\n", pc, instr_to_string(&instr));
            abort();
        }

#ifdef DEBUG
        if (unlikely(!machine->skip_breakpoint && machine_check_breakpoint(machine, pc))) {
            cpu_set_flow_ctl(state, FLOW_HALT);
            break;
        }
#endif
        cpu_increase_flow_pc(state, instr.rvc ? 2 : 4);

        InstrFunc func = dispatch_table[instr.kind];
        func(state, &instr);

#ifdef DEBUG
        machine->skip_breakpoint = false;
#endif

        if (instr.cfc) {
            u64 *count = ht_find(&record, pc);
            if (count)
                *count += 1;
            else
                *ht_put(&record, pc) = 1;
            break;
        }
        cpu_commit_pc(state);
    }
}

