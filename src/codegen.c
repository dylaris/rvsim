#include "codegen.h"
#include "mmu.h"

static __ForceInline void emit_lb(u8 **dst, CPUState *state, Instr *instr)
{
    u64 addr = state->gp_regs[instr->rs1] + (i64) instr->imm;
    state->gp_regs[instr->rd] = *(u8 *) (addr + GUEST_MEMORY_OFFSET);
}

static __ForceInline void emit_lh(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_lw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_ld(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_lbu(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_lhu(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_lwu(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_fence(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_fence_i(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_ebreak(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_auipc(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_lui(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_ecall(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_addi(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_slli(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_slti(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_sltiu(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_xori(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_srli(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_srai(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_ori(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_andi(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_addiw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_slliw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_srliw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_sraiw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_sb(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_sh(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_sw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_sd(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_add(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_sll(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_slt(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_sltu(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_xor(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_srl(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_or(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_and(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_mul(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_mulh(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_mulhsu(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_mulhu(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_div(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_divu(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_rem(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_remu(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_sub(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_sra(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_addw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_sllw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_srlw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_mulw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_divw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_divuw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_remw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_remuw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_subw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_sraw(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_beq(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_bne(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_blt(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_bge(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_bltu(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_bgeu(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_jalr(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
}

static __ForceInline void emit_jal(u8 **dst, CPUState *state, Instr *instr)
{
    (void) dst;
    (void) state;
    (void) instr;
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
    [instr_mul]       = emit_mul,
    [instr_mulh]      = emit_mulh,
    [instr_mulhsu]    = emit_mulhsu,
    [instr_mulhu]     = emit_mulhu,
    [instr_div]       = emit_div,
    [instr_divu]      = emit_divu,
    [instr_rem]       = emit_rem,
    [instr_remu]      = emit_remu,
    [instr_sub]       = emit_sub,
    [instr_sra]       = emit_sra,
    [instr_addw]      = emit_addw,
    [instr_sllw]      = emit_sllw,
    [instr_srlw]      = emit_srlw,
    [instr_mulw]      = emit_mulw,
    [instr_divw]      = emit_divw,
    [instr_divuw]     = emit_divuw,
    [instr_remw]      = emit_remw,
    [instr_remuw]     = emit_remuw,
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
    [instr_csrrc]     = NULL,
    [instr_csrrci]    = NULL,
    [instr_csrrs]     = NULL,
    [instr_csrrsi]    = NULL,
    [instr_csrrw]     = NULL,
    [instr_csrrwi]    = NULL,
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
