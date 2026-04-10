#include "codegen.h"
#include "mmu.h"
#include "reg.h"
#include "uop.h"

typedef u64 (*EmitFunc)(u8 **, CPUState *, Instr *, u64);

#define REG_OFFSET(idx) ((i64) ((char *) state->gp_regs[idx] - (char *) state->gp_regs))
#define ADDRESS(imm) ((i64) mmu_to_host((i64) (imm)))

// Helper function to write bytes in little-endian order
static __ForceInline void write_le_bytes(u8 *dst, u64 value, u64 num_bytes)
{
    for (u64 i = 0; i < num_bytes; ++i)
        dst[i] = (value >> (i * 8)) & 0xFF;
}

static __ForceInline void emit_uop(u8 **dst, UOP *uop, i64 imm)
{
    memcpy(*dst, uop->code, uop->length);
    if (uop->patch_length)
        write_le_bytes(uop->code + uop->patch_offset, (u64) imm, uop->patch_length);
    *dst += uop->length;
}

#define EMPTY \
    (void) dst; \
    (void) state; \
    (void) instr; \
    (void) pc; \
    return 0;

#define FUNC(type) \
    (void) pc; \
    u8 *start = *dst; \
    emit_uop(dst, &uop_move_reg_t1, REG_OFFSET(instr->rs1)); \
    emit_uop(dst, &uop_load_##type, ADDRESS(instr->imm)); \
    emit_uop(dst, &uop_move_t0_reg, REG_OFFSET(instr->rd)); \
    return (u64) (*dst - start);


static __ForceInline u64 emit_lb(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    FUNC(s8)
}

static __ForceInline u64 emit_lh(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    FUNC(s16)
}

static __ForceInline u64 emit_lw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    FUNC(s32)
}

static __ForceInline u64 emit_ld(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    FUNC(u64)
}

static __ForceInline u64 emit_lbu(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    FUNC(u8)
}

static __ForceInline u64 emit_lhu(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    FUNC(u16)
}

static __ForceInline u64 emit_lwu(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    FUNC(u32)
}

#undef FUNC

static __ForceInline u64 emit_fence(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_fence_i(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_ebreak(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_auipc(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    u8 *start = *dst;
    u64 val = pc + (i64) instr->imm;
    emit_uop(dst, &uop_move_imm64_t0, (i64) val);
    emit_uop(dst, &uop_move_t0_reg, REG_OFFSET(instr->rd));
    return *dst - start;
}

static __ForceInline u64 emit_lui(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    (void) pc;
    u8 *start = *dst;
    emit_uop(dst, &uop_move_imm64_t0, (i64) instr->imm);
    emit_uop(dst, &uop_move_t0_reg, REG_OFFSET(instr->rd));
    return *dst - start;
}

static __ForceInline u64 emit_ecall(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    (void) dst;
    (void) instr;
    cpu_set_flow_ctl(state, FLOW_ECALL);
    cpu_set_flow_pc(state, pc + 4);
    return 0;
}

static __ForceInline u64 emit_addi(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    (void) pc;
    u8 *start = *dst;
    emit_uop(dst, &uop_move_reg_t0, REG_OFFSET(instr->rs1));
    emit_uop(dst, &uop_move_imm64_t1, (i64) instr->imm);
    emit_uop(dst, &uop_add_t0_t1, 0);
    emit_uop(dst, &uop_move_t0_reg, REG_OFFSET(instr->rd));
    return *dst - start;
}

static __ForceInline u64 emit_slli(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    (void) pc;
    u8 *start = *dst;
    emit_uop(dst, &uop_move_reg_t0, REG_OFFSET(instr->rs1));
    emit_uop(dst, &uop_move_imm32_t2, (i64) instr->imm & 0x3f);
    emit_uop(dst, &uop_sll_t0_t2, 0);
    emit_uop(dst, &uop_move_t0_reg, REG_OFFSET(instr->rd));
    return *dst - start;
}

static __ForceInline u64 emit_slti(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    (void) pc;
    u8 *start = *dst;
    emit_uop(dst, &uop_move_reg_t0, REG_OFFSET(instr->rs1));
    emit_uop(dst, &uop_move_imm64_t1, (i64) instr->imm);
    emit_uop(dst, &uop_slt_t0_t1, 0);
    emit_uop(dst, &uop_move_t0_reg, REG_OFFSET(instr->rd));
    return *dst - start;
}

static __ForceInline u64 emit_sltiu(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    (void) pc;
    u8 *start = *dst;
    emit_uop(dst, &uop_move_reg_t0, REG_OFFSET(instr->rs1));
    emit_uop(dst, &uop_move_imm64_t1, (u64) instr->imm);
    emit_uop(dst, &uop_sltu_t0_t1, 0);
    emit_uop(dst, &uop_move_t0_reg, REG_OFFSET(instr->rd));
    return *dst - start;
}

static __ForceInline u64 emit_xori(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    (void) pc;
    u8 *start = *dst;
    emit_uop(dst, &uop_move_reg_t0, REG_OFFSET(instr->rs1));
    emit_uop(dst, &uop_move_imm64_t1, (i64) instr->imm);
    emit_uop(dst, &uop_xor_t0_t1, 0);
    emit_uop(dst, &uop_move_t0_reg, REG_OFFSET(instr->rd));
    return *dst - start;
}

static __ForceInline u64 emit_srli(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    (void) pc;
    u8 *start = *dst;
    emit_uop(dst, &uop_move_reg_t0, REG_OFFSET(instr->rs1));
    emit_uop(dst, &uop_move_imm32_t2, (i64) instr->imm & 0x3f);
    emit_uop(dst, &uop_srl_t0_t2, 0);
    emit_uop(dst, &uop_move_t0_reg, REG_OFFSET(instr->rd));
    return *dst - start;
}

static __ForceInline u64 emit_srai(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    (void) pc;
    u8 *start = *dst;
    emit_uop(dst, &uop_move_reg_t0, REG_OFFSET(instr->rs1));
    emit_uop(dst, &uop_move_imm32_t2, (i64) instr->imm & 0x3f);
    emit_uop(dst, &uop_sra_t0_t2, 0);
    emit_uop(dst, &uop_move_t0_reg, REG_OFFSET(instr->rd));
    return *dst - start;
}

static __ForceInline u64 emit_ori(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    (void) pc;
    u8 *start = *dst;
    emit_uop(dst, &uop_move_reg_t0, REG_OFFSET(instr->rs1));
    emit_uop(dst, &uop_move_imm64_t1, (u64)(i64) instr->imm);
    emit_uop(dst, &uop_or_t0_t1, 0);
    emit_uop(dst, &uop_move_t0_reg, REG_OFFSET(instr->rd));
    return *dst - start;
}

static __ForceInline u64 emit_andi(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    (void) pc;
    u8 *start = *dst;
    emit_uop(dst, &uop_move_reg_t0, REG_OFFSET(instr->rs1));
    emit_uop(dst, &uop_move_imm64_t1, (u64)(i64) instr->imm);
    emit_uop(dst, &uop_and_t0_t1, 0);
    emit_uop(dst, &uop_move_t0_reg, REG_OFFSET(instr->rd));
    return *dst - start;
}

static __ForceInline u64 emit_addiw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_slliw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_srliw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_sraiw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_sb(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_sh(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_sw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_sd(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_add(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_sll(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_slt(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_sltu(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_xor(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_srl(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_or(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_and(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline __Keep u64 emit_mul(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline __Keep u64 emit_mulh(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline __Keep u64 emit_mulhsu(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline __Keep u64 emit_mulhu(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline __Keep u64 emit_div(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline __Keep u64 emit_divu(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline __Keep u64 emit_rem(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline __Keep u64 emit_remu(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_sub(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_sra(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_addw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_sllw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_srlw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline __Keep u64 emit_mulw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline __Keep u64 emit_divw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline __Keep u64 emit_divuw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline __Keep u64 emit_remw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline __Keep u64 emit_remuw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_subw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_sraw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_beq(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_bne(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_blt(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_bge(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_bltu(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_bgeu(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_jalr(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_jal(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_csrrc(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_csrrci(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_csrrs(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_csrrsi(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_csrrw(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

static __ForceInline u64 emit_csrrwi(u8 **dst, CPUState *state, Instr *instr, u64 pc)
{
    EMPTY
}

EmitFunc emit_table[] = {
    [instr_lb]        = emit_lb,
    [instr_lh]        = emit_lh,
    [instr_lw]        = emit_lw,
    [instr_ld]        = emit_ld,
    [instr_lbu]       = emit_lbu,
    [instr_lhu]       = emit_lhu,
    [instr_lwu]       = emit_lwu,
    [instr_fence]     = emit_fence,
    [instr_fence_i]   = emit_fence_i,
    [instr_ebreak]    = emit_ebreak,
    [instr_auipc]     = emit_auipc,
    [instr_lui]       = emit_lui,
    [instr_ecall]     = emit_ecall,
    [instr_addi]      = emit_addi,
    [instr_slli]      = emit_slli,
    [instr_slti]      = emit_slti,
    [instr_sltiu]     = emit_sltiu,
    [instr_xori]      = emit_xori,
    [instr_srli]      = emit_srli,
    [instr_srai]      = emit_srai,
    [instr_ori]       = emit_ori,
    [instr_andi]      = emit_andi,
    [instr_addiw]     = emit_addiw,
    [instr_slliw]     = emit_slliw,
    [instr_srliw]     = emit_srliw,
    [instr_sraiw]     = emit_sraiw,
    [instr_sb]        = emit_sb,
    [instr_sh]        = emit_sh,
    [instr_sw]        = emit_sw,
    [instr_sd]        = emit_sd,
    [instr_add]       = emit_add,
    [instr_sll]       = emit_sll,
    [instr_slt]       = emit_slt,
    [instr_sltu]      = emit_sltu,
    [instr_xor]       = emit_xor,
    [instr_srl]       = emit_srl,
    [instr_or]        = emit_or,
    [instr_and]       = emit_and,
    [instr_mul]       = NULL,
    [instr_mulh]      = NULL,
    [instr_mulhsu]    = NULL,
    [instr_mulhu]     = NULL,
    [instr_div]       = NULL,
    [instr_divu]      = NULL,
    [instr_rem]       = NULL,
    [instr_remu]      = NULL,
    [instr_sub]       = emit_sub,
    [instr_sra]       = emit_sra,
    [instr_addw]      = emit_addw,
    [instr_sllw]      = emit_sllw,
    [instr_srlw]      = emit_srlw,
    [instr_mulw]      = NULL,
    [instr_divw]      = NULL,
    [instr_divuw]     = NULL,
    [instr_remw]      = NULL,
    [instr_remuw]     = NULL,
    [instr_subw]      = emit_subw,
    [instr_sraw]      = emit_sraw,
    [instr_beq]       = emit_beq,
    [instr_bne]       = emit_bne,
    [instr_blt]       = emit_blt,
    [instr_bge]       = emit_bge,
    [instr_bltu]      = emit_bltu,
    [instr_bgeu]      = emit_bgeu,
    [instr_jalr]      = emit_jalr,
    [instr_jal]       = emit_jal,
    [instr_csrrc]     = emit_csrrc,
    [instr_csrrci]    = emit_csrrci,
    [instr_csrrs]     = emit_csrrs,
    [instr_csrrsi]    = emit_csrrsi,
    [instr_csrrw]     = emit_csrrw,
    [instr_csrrwi]    = emit_csrrwi,
    [instr_flw]       = NULL,
    [instr_fld]       = NULL,
    [instr_fsw]       = NULL,
    [instr_fsd]       = NULL,
    [instr_fmadd_s]   = NULL,
    [instr_fmsub_s]   = NULL,
    [instr_fnmsub_s]  = NULL,
    [instr_fnmadd_s]  = NULL,
    [instr_fmadd_d]   = NULL,
    [instr_fmsub_d]   = NULL,
    [instr_fnmsub_d]  = NULL,
    [instr_fnmadd_d]  = NULL,
    [instr_fadd_s]    = NULL,
    [instr_fsub_s]    = NULL,
    [instr_fmul_s]    = NULL,
    [instr_fdiv_s]    = NULL,
    [instr_fsqrt_s]   = NULL,
    [instr_fmin_s]    = NULL,
    [instr_fmax_s]    = NULL,
    [instr_fadd_d]    = NULL,
    [instr_fsub_d]    = NULL,
    [instr_fmul_d]    = NULL,
    [instr_fdiv_d]    = NULL,
    [instr_fsqrt_d]   = NULL,
    [instr_fmin_d]    = NULL,
    [instr_fmax_d]    = NULL,
    [instr_fsgnj_s]   = NULL,
    [instr_fsgnjn_s]  = NULL,
    [instr_fsgnjx_s]  = NULL,
    [instr_fsgnj_d]   = NULL,
    [instr_fsgnjn_d]  = NULL,
    [instr_fsgnjx_d]  = NULL,
    [instr_fcvt_w_s]  = NULL,
    [instr_fcvt_wu_s] = NULL,
    [instr_fcvt_l_s]  = NULL,
    [instr_fcvt_lu_s] = NULL,
    [instr_fcvt_w_d]  = NULL,
    [instr_fcvt_wu_d] = NULL,
    [instr_fcvt_l_d]  = NULL,
    [instr_fcvt_lu_d] = NULL,
    [instr_fmv_x_w]   = NULL,
    [instr_fmv_x_d]   = NULL,
    [instr_fcvt_s_w]  = NULL,
    [instr_fcvt_s_wu] = NULL,
    [instr_fcvt_s_l]  = NULL,
    [instr_fcvt_s_lu] = NULL,
    [instr_fcvt_d_w]  = NULL,
    [instr_fcvt_d_wu] = NULL,
    [instr_fcvt_d_l]  = NULL,
    [instr_fcvt_d_lu] = NULL,
    [instr_fmv_w_x]   = NULL,
    [instr_fmv_d_x]   = NULL,
    [instr_fcvt_s_d]  = NULL,
    [instr_fcvt_d_s]  = NULL,
    [instr_feq_s]     = NULL,
    [instr_flt_s]     = NULL,
    [instr_fle_s]     = NULL,
    [instr_feq_d]     = NULL,
    [instr_flt_d]     = NULL,
    [instr_fle_d]     = NULL,
    [instr_fclass_s]  = NULL,
    [instr_fclass_d]  = NULL,
};

u8 *gen_block(Machine *machine, u64 pc, u64 cache_entry_index)
{
    CacheEntry *entry = &machine->cache.entries[cache_entry_index];
    entry->offset = ROUNDUP(machine->cache.end, 2);
    u8 *code = machine->cache.jitcode + entry->offset;

    while (true) {
        Instr instr = {0};

        u32 raw = mem_read_u32(pc);
        decode_instr(raw, &instr);

        EmitFunc emit = emit_table[instr.kind];
        if (!emit) // not support emit for this instruction
            break;
        u64 length = emit(&code, &machine->state, &instr, pc);
        entry->length += length;
        entry->instr_count++;

        if (instr.cfc)
            break;
        pc += instr.rvc ? 2 : 4;
    }

    machine->cache.end = ROUNDUP(entry->offset + entry->length, 2);

    return cache_code(&machine->cache, cache_entry_index);
}

