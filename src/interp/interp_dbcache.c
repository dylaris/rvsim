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
        if (!entry->branch_taken) { \
            entry->branch_taken = dbcache_get(dbcache, cpu_get_pc(state)); \
        } \
        entry = entry->branch_taken; \
    } else { \
        cpu_set_flow(state, FLOW_BRANCH_NOT_TAKEN); \
        if (!entry->branch_not_taken) { \
            entry->branch_not_taken = dbcache_get(dbcache, cpu_get_pc(state)); \
        } \
        entry = entry->branch_not_taken; \
    } \
    EXEC_BLOCK(entry); \
}

#define GEN_JUMP(name, addr, a2, a3, a4) \
SIGNATURE(name) \
{ \
    u64 rs1 = cpu_get_gpr(state, instr->rs1); \
    i64 imm = (i64) instr->imm; \
    u64 pc = instr->curr_pc; \
    cpu_set_gpr(state, instr->rd, instr->next_pc); \
    cpu_set_pc(state, addr); \
    if (instr->kind == instr_jal) { \
        cpu_set_flow(state, FLOW_DIRECT_JUMP); \
        if (!entry->jal_target) { \
            entry->jal_target = dbcache_get(dbcache, cpu_get_pc(state)); \
        } \
        entry = entry->jal_target; \
    } else { \
        cpu_set_flow(state, FLOW_INDIRECT_JUMP); \
        DBCacheEntry *target_entry = NULL; \
        for (int i = 0; i < DYN_LINK_CACHE_SIZE; i++) { \
            if (entry->dyn_link_cache[i].target_pc == cpu_get_pc(state)) { \
                target_entry = entry->dyn_link_cache[i].target_entry; \
                break; \
            } \
        } \
        if (!target_entry) { \
            entry->dyn_link_cache[entry->dyn_link_next].target_pc = cpu_get_pc(state); \
            entry->dyn_link_cache[entry->dyn_link_next].target_entry = dbcache_get(dbcache, cpu_get_pc(state)); \
            target_entry = entry->dyn_link_cache[entry->dyn_link_next].target_entry; \
            entry->dyn_link_next = (entry->dyn_link_next + 1) % DYN_LINK_CACHE_SIZE; \
        } \
        entry = target_entry; \
    } \
    EXEC_BLOCK(entry); \
}

#define GEN_ECALL(name, a1, a2, a3, a4) \
SIGNATURE(name) \
{ \
    cpu_set_flow(state, FLOW_ECALL); \
    cpu_set_pc(state, instr->next_pc); \
    return; \
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

#define SIGNATURE(name) name:
#define GEN(name, tag, a1, a2, a3, a4) \
    GEN_##tag(name, a1, a2, a3, a4) \
    instr++; \
    if (instr >= block_end) goto end_of_block; \
    goto *dispatch_table_dbcache[instr->kind];

// NOTE:
// Remember not to modify the fields of instr pointer, because it
// stores the decode cache, which will reuse by others in future.
// And also, each block entry has its own link fields, which means
// all blocks (exclude ecall) will link directly, and it will
// exit when encounter ecall (return in ecall label)
// By the way, the link behavior is in the end of GEN_BRANCH/GEN_JUMP
void interp(Machine *machine)
{
    CPUState *state = &machine->state;
    DBCache *dbcache = machine->dbcache;
    DBCacheEntry *entry = NULL;
    Instr single_instr;
    Instr *instr = NULL;
    Instr *block_end = NULL;

#define X(name, tag, a1, a2, a3, a4) [instr_##name] = &&name,
    static void *dispatch_table_dbcache[] = { INSTRUCTION_LIST(X) };
#undef X

#define EXEC_BLOCK(e) \
    do { \
        instr = &(e)->items[0]; \
        block_end = instr + (e)->count; \
        cpu_set_pc(state, block_end[-1].next_pc); \
        goto *dispatch_table_dbcache[instr->kind]; \
    } while (0)

    while (true) {
        // I don't know why it will faster a little bit
        // when adding the end_of_block label and this loop.
        // It should be execute in the "goto"s beacuse the
        // graph of blocks, and exit in ecall
        entry = dbcache_get(dbcache, cpu_get_pc(state));
        EXEC_BLOCK(entry);

end_of_block:
        ;
    }

    INSTRUCTION_LIST(GEN)
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

