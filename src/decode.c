#include "decode.h"

#define QUADRANT(raw) (((raw) >>  0) & 0x3 )

/*
 * normal types
 */
#define OPCODE(raw) (((raw) >>  2) & 0x1f)
#define RD(raw)     (((raw) >>  7) & 0x1f)
#define RS1(raw)    (((raw) >> 15) & 0x1f)
#define RS2(raw)    (((raw) >> 20) & 0x1f)
#define RS3(raw)    (((raw) >> 27) & 0x1f)
#define FUNCT2(raw) (((raw) >> 25) & 0x3 )
#define FUNCT3(raw) (((raw) >> 12) & 0x7 )
#define FUNCT7(raw) (((raw) >> 25) & 0x7f)
#define IMM116(raw) (((raw) >> 26) & 0x3f)

static __ForceInline Instr instr_utype_read(u32 raw)
{
    return (Instr) {
        .imm = (i32)raw & 0xfffff000,
        .rd = RD(raw),
    };
}

static __ForceInline Instr instr_itype_read(u32 raw)
{
    return (Instr) {
        .imm = (i32)raw >> 20,
        .rs1 = RS1(raw),
        .rd = RD(raw),
    };
}

static __ForceInline Instr instr_jtype_read(u32 raw)
{
    u32 imm20   = (raw >> 31) & 0x1;
    u32 imm101  = (raw >> 21) & 0x3ff;
    u32 imm11   = (raw >> 20) & 0x1;
    u32 imm1912 = (raw >> 12) & 0xff;

    i32 imm = (imm20 << 20) | (imm1912 << 12) | (imm11 << 11) | (imm101 << 1);
    imm = (imm << 11) >> 11;

    return (Instr) {
        .imm = imm,
        .rd = RD(raw),
    };
}

static __ForceInline Instr instr_btype_read(u32 raw)
{
    u32 imm12  = (raw >> 31) & 0x1;
    u32 imm105 = (raw >> 25) & 0x3f;
    u32 imm41  = (raw >>  8) & 0xf;
    u32 imm11  = (raw >>  7) & 0x1;

    i32 imm = (imm12 << 12) | (imm11 << 11) |(imm105 << 5) | (imm41 << 1);
    imm = (imm << 19) >> 19;

    return (Instr) {
        .imm = imm,
        .rs1 = RS1(raw),
        .rs2 = RS2(raw),
    };
}

static __ForceInline Instr instr_rtype_read(u32 raw)
{
    return (Instr) {
        .rs1 = RS1(raw),
        .rs2 = RS2(raw),
        .rd = RD(raw),
    };
}

static __ForceInline Instr instr_stype_read(u32 raw)
{
    u32 imm115 = (raw >> 25) & 0x7f;
    u32 imm40  = (raw >>  7) & 0x1f;

    i32 imm = (imm115 << 5) | imm40;
    imm = (imm << 20) >> 20;
    return (Instr) {
        .imm = imm,
        .rs1 = RS1(raw),
        .rs2 = RS2(raw),
    };
}

static __ForceInline Instr instr_csrtype_read(u32 raw)
{
    return (Instr) {
        .imm = raw >> 20,
        .rs1 = RS1(raw),
        .rd =  RD(raw),
    };
}

static __ForceInline Instr instr_fprtype_read(u32 raw)
{
    return (Instr) {
        .rs1 = RS1(raw),
        .rs2 = RS2(raw),
        .rs3 = RS3(raw),
        .rd =  RD(raw),
    };
}

/*
 * compressed types
 */
#define COPCODE(raw)     (((raw) >> 13) & 0x7 )
#define CFUNCT1(raw)     (((raw) >> 12) & 0x1 )
#define CFUNCT2LOW(raw)  (((raw) >>  5) & 0x3 )
#define CFUNCT2HIGH(raw) (((raw) >> 10) & 0x3 )
#define RP1(raw)         (((raw) >>  7) & 0x7 )
#define RP2(raw)         (((raw) >>  2) & 0x7 )
#define RC1(raw)         (((raw) >>  7) & 0x1f)
#define RC2(raw)         (((raw) >>  2) & 0x1f)

static __ForceInline Instr instr_catype_read(u16 raw)
{
    return (Instr) {
        .rd = RP1(raw) + 8,
        .rs2 = RP2(raw) + 8,
        .rvc = true,
    };
}

static __ForceInline Instr instr_crtype_read(u16 raw)
{
    return (Instr) {
        .rs1 = RC1(raw),
        .rs2 = RC2(raw),
        .rvc = true,
    };
}

static __ForceInline Instr instr_citype_read(u16 raw)
{
    u32 imm40 = (raw >>  2) & 0x1f;
    u32 imm5  = (raw >> 12) & 0x1;
    i32 imm = (imm5 << 5) | imm40;
    imm = (imm << 26) >> 26;

    return (Instr) {
        .imm = imm,
        .rd = RC1(raw),
        .rvc = true,
    };
}

static __ForceInline Instr instr_citype_read2(u16 raw)
{
    u32 imm86 = (raw >>  2) & 0x7;
    u32 imm43 = (raw >>  5) & 0x3;
    u32 imm5  = (raw >> 12) & 0x1;

    i32 imm = (imm86 << 6) | (imm43 << 3) | (imm5 << 5);

    return (Instr) {
        .imm = imm,
        .rd = RC1(raw),
        .rvc = true,
    };
}

static __ForceInline Instr instr_citype_read3(u16 raw)
{
    u32 imm5  = (raw >>  2) & 0x1;
    u32 imm87 = (raw >>  3) & 0x3;
    u32 imm6  = (raw >>  5) & 0x1;
    u32 imm4  = (raw >>  6) & 0x1;
    u32 imm9  = (raw >> 12) & 0x1;

    i32 imm = (imm5 << 5) | (imm87 << 7) | (imm6 << 6) | (imm4 << 4) | (imm9 << 9);
    imm = (imm << 22) >> 22;

    return (Instr) {
        .imm = imm,
        .rd = RC1(raw),
        .rvc = true,
    };
}

static __ForceInline Instr instr_citype_read4(u16 raw)
{
    u32 imm5  = (raw >> 12) & 0x1;
    u32 imm42 = (raw >>  4) & 0x7;
    u32 imm76 = (raw >>  2) & 0x3;

    i32 imm = (imm5 << 5) | (imm42 << 2) | (imm76 << 6);

    return (Instr) {
        .imm = imm,
        .rd = RC1(raw),
        .rvc = true,
    };
}

static __ForceInline Instr instr_citype_read5(u16 raw)
{
    u32 imm1612 = (raw >>  2) & 0x1f;
    u32 imm17   = (raw >> 12) & 0x1;

    i32 imm = (imm1612 << 12) | (imm17 << 17);
    imm = (imm << 14) >> 14;
    return (Instr) {
        .imm = imm,
        .rd = RC1(raw),
        .rvc = true,
    };
}

static __ForceInline Instr instr_cbtype_read(u16 raw)
{
    u32 imm5  = (raw >>  2) & 0x1;
    u32 imm21 = (raw >>  3) & 0x3;
    u32 imm76 = (raw >>  5) & 0x3;
    u32 imm43 = (raw >> 10) & 0x3;
    u32 imm8  = (raw >> 12) & 0x1;

    i32 imm = (imm8 << 8) | (imm76 << 6) | (imm5 << 5) | (imm43 << 3) | (imm21 << 1);
    imm = (imm << 23) >> 23;

    return (Instr) {
        .imm = imm,
        .rs1 = RP1(raw) + 8,
        .rvc = true,
    };
}

static __ForceInline Instr instr_cbtype_read2(u16 raw)
{
    u32 imm40 = (raw >>  2) & 0x1f;
    u32 imm5  = (raw >> 12) & 0x1;
    i32 imm = (imm5 << 5) | imm40;
    imm = (imm << 26) >> 26;

    return (Instr) {
        .imm = imm,
        .rd = RP1(raw) + 8,
        .rvc = true,
    };
}

static __ForceInline Instr instr_cstype_read(u16 raw)
{
    u32 imm76 = (raw >>  5) & 0x3;
    u32 imm53 = (raw >> 10) & 0x7;

    i32 imm = ((imm76 << 6) | (imm53 << 3));

    return (Instr) {
        .imm = imm,
        .rs1 = RP1(raw) + 8,
        .rs2 = RP2(raw) + 8,
        .rvc = true,
    };
}

static __ForceInline Instr instr_cstype_read2(u16 raw)
{
    u32 imm6  = (raw >>  5) & 0x1;
    u32 imm2  = (raw >>  6) & 0x1;
    u32 imm53 = (raw >> 10) & 0x7;

    i32 imm = ((imm6 << 6) | (imm2 << 2) | (imm53 << 3));

    return (Instr) {
        .imm = imm,
        .rs1 = RP1(raw) + 8,
        .rs2 = RP2(raw) + 8,
        .rvc = true,
    };
}

static __ForceInline Instr instr_cjtype_read(u16 raw)
{
    u32 imm5  = (raw >>  2) & 0x1;
    u32 imm31 = (raw >>  3) & 0x7;
    u32 imm7  = (raw >>  6) & 0x1;
    u32 imm6  = (raw >>  7) & 0x1;
    u32 imm10 = (raw >>  8) & 0x1;
    u32 imm98 = (raw >>  9) & 0x3;
    u32 imm4  = (raw >> 11) & 0x1;
    u32 imm11 = (raw >> 12) & 0x1;

    i32 imm = ((imm5 << 5) | (imm31 << 1) | (imm7 << 7) | (imm6 << 6) |
               (imm10 << 10) | (imm98 << 8) | (imm4 << 4) | (imm11 << 11));
    imm = (imm << 20) >> 20;
    return (Instr) {
        .imm = imm,
        .rvc = true,
    };
}

static __ForceInline Instr instr_cltype_read(u16 raw)
{
    u32 imm6  = (raw >>  5) & 0x1;
    u32 imm2  = (raw >>  6) & 0x1;
    u32 imm53 = (raw >> 10) & 0x7;

    i32 imm = (imm6 << 6) | (imm2 << 2) | (imm53 << 3);

    return (Instr) {
        .imm = imm,
        .rs1 = RP1(raw) + 8,
        .rd  = RP2(raw) + 8,
        .rvc = true,
    };
}

static __ForceInline Instr instr_cltype_read2(u16 raw)
{
    u32 imm76 = (raw >>  5) & 0x3;
    u32 imm53 = (raw >> 10) & 0x7;

    i32 imm = (imm76 << 6) | (imm53 << 3);

    return (Instr) {
        .imm = imm,
        .rs1 = RP1(raw) + 8,
        .rd  = RP2(raw) + 8,
        .rvc = true,
    };
}

static __ForceInline Instr instr_csstype_read(u16 raw)
{
    u32 imm86 = (raw >>  7) & 0x7;
    u32 imm53 = (raw >> 10) & 0x7;

    i32 imm = (imm86 << 6) | (imm53 << 3);

    return (Instr) {
        .imm = imm,
        .rs2 = RC2(raw),
        .rvc = true,
    };
}

static __ForceInline Instr instr_csstype_read2(u16 raw)
{
    u32 imm76 = (raw >> 7) & 0x3;
    u32 imm52 = (raw >> 9) & 0xf;

    i32 imm = (imm76 << 6) | (imm52 << 2);

    return (Instr) {
        .imm = imm,
        .rs2 = RC2(raw),
        .rvc = true,
    };
}

static __ForceInline Instr instr_ciwtype_read(u16 raw)
{
    u32 imm3  = (raw >>  5) & 0x1;
    u32 imm2  = (raw >>  6) & 0x1;
    u32 imm96 = (raw >>  7) & 0xf;
    u32 imm54 = (raw >> 11) & 0x3;

    i32 imm = (imm3 << 3) | (imm2 << 2) | (imm96 << 6) | (imm54 << 4);

    return (Instr) {
        .imm = imm,
        .rd = RP2(raw) + 8,
        .rvc = true,
    };
}

bool decode_instr(u32 raw, Instr *out)
{
    u32 quadrant = QUADRANT(raw);
    switch (quadrant) {
    case 0x0: {
        u32 copcode = COPCODE(raw);

        switch (copcode) {
        case 0x0: /* C.ADDI4SPN */
            *out = instr_ciwtype_read(raw);
            out->rs1 = GPR_SP;
            out->kind = instr_addi;
            if (unlikely(out->imm == 0))
                return false;
            return true;
        case 0x1: /* C.FLD */
            *out = instr_cltype_read2(raw);
            out->kind = instr_fld;
            return true;
        case 0x2: /* C.LW */
            *out = instr_cltype_read(raw);
            out->kind = instr_lw;
            return true;
        case 0x3: /* C.LD */
            *out = instr_cltype_read2(raw);
            out->kind = instr_ld;
            return true;
        case 0x5: /* C.FSD */
            *out = instr_cstype_read(raw);
            out->kind = instr_fsd;
            return true;
        case 0x6: /* C.SW */
            *out = instr_cstype_read2(raw);
            out->kind = instr_sw;
            return true;
        case 0x7: /* C.SD */
            *out = instr_cstype_read(raw);
            out->kind = instr_sd;
            return true;
        default:
            printf("raw: %x\n", raw);
            fatal("unimplemented");
        }
    }
    unreachable();
    case 0x1: {
        u32 copcode = COPCODE(raw);

        switch (copcode) {
        case 0x0: /* C.ADDI */
            *out = instr_citype_read(raw);
            out->rs1 = out->rd;
            out->kind = instr_addi;
            return true;
        case 0x1: /* C.ADDIW */
            *out = instr_citype_read(raw);
            if (unlikely(out->rd == 0))
                return false;
            out->rs1 = out->rd;
            out->kind = instr_addiw;
            return true;
        case 0x2: /* C.LI */
            *out = instr_citype_read(raw);
            out->rs1 = GPR_ZERO;
            out->kind = instr_addi;
            return true;
        case 0x3: {
            i32 rd = RC1(raw);
            if (rd == 2) { /* C.ADDI16SP */
                *out = instr_citype_read3(raw);
                if (unlikely(out->imm == 0))
                    return false;
                out->rs1 = out->rd;
                out->kind = instr_addi;
                return true;
            } else { /* C.LUI */
                *out = instr_citype_read5(raw);
                if (unlikely(out->imm == 0))
                    return false;
                out->kind = instr_lui;
                return true;
            }
        }
        unreachable();
        case 0x4: {
            u32 cfunct2high = CFUNCT2HIGH(raw);

            switch (cfunct2high) {
            case 0x0:   /* C.SRLI */
            case 0x1:   /* C.SRAI */
            case 0x2: { /* C.ANDI */
                *out = instr_cbtype_read2(raw);
                out->rs1 = out->rd;

                if (cfunct2high == 0x0) {
                    out->kind = instr_srli;
                } else if (cfunct2high == 0x1) {
                    out->kind = instr_srai;
                } else {
                    out->kind = instr_andi;
                }
                return true;
            }
            unreachable();
            case 0x3: {
                u32 cfunct1 = CFUNCT1(raw);

                switch (cfunct1) {
                case 0x0: {
                    u32 cfunct2low = CFUNCT2LOW(raw);

                    *out = instr_catype_read(raw);
                    out->rs1 = out->rd;

                    switch (cfunct2low) {
                    case 0x0: /* C.SUB */
                        out->kind = instr_sub;
                        break;
                    case 0x1: /* C.XOR */
                        out->kind = instr_xor;
                        break;
                    case 0x2: /* C.OR */
                        out->kind = instr_or;
                        break;
                    case 0x3: /* C.AND */
                        out->kind = instr_and;
                        break;
                    default: unreachable();
                    }
                    return true;
                }
                unreachable();
                case 0x1: {
                    u32 cfunct2low = CFUNCT2LOW(raw);

                    *out = instr_catype_read(raw);
                    out->rs1 = out->rd;

                    switch (cfunct2low) {
                    case 0x0: /* C.SUBW */
                        out->kind = instr_subw;
                        break;
                    case 0x1: /* C.ADDW */
                        out->kind = instr_addw;
                        break;
                    default: unreachable();
                    }
                    return true;
                }
                unreachable();
                default: unreachable();
                }
            }
            unreachable();
            default: unreachable();
            }
        }
        unreachable();
        case 0x5: /* C.J */
            *out = instr_cjtype_read(raw);
            out->rd = GPR_ZERO;
            out->kind = instr_jal;
            return true;
        case 0x6: /* C.BEQZ */
        case 0x7: /* C.BNEZ */
            *out = instr_cbtype_read(raw);
            out->rs2 = GPR_ZERO;
            out->kind = copcode == 0x6 ? instr_beq : instr_bne;
            out->cfc = true;
            return true;
        default:
            fatal("unrecognized copcode");
        }
    }
    unreachable();
    case 0x2: {
        u32 copcode = COPCODE(raw);
        switch (copcode) {
        case 0x0: /* C.SLLI */
            *out = instr_citype_read(raw);
            out->rs1 = out->rd;
            out->kind = instr_slli;
            return true;
        case 0x1: /* C.FLDSP */
            *out = instr_citype_read2(raw);
            out->rs1 = GPR_SP;
            out->kind = instr_fld;
            return true;
        case 0x2: /* C.LWSP */
            *out = instr_citype_read4(raw);
            if (unlikely(out->rd == 0))
                return false;
            out->rs1 = GPR_SP;
            out->kind = instr_lw;
            return true;
        case 0x3: /* C.LDSP */
            *out = instr_citype_read2(raw);
            if (unlikely(out->rd == 0))
                return false;
            out->rs1 = GPR_SP;
            out->kind = instr_ld;
            return true;
        case 0x4: {
            u32 cfunct1 = CFUNCT1(raw);

            switch (cfunct1) {
            case 0x0: {
                *out = instr_crtype_read(raw);

                if (out->rs2 == 0) { /* C.JR */
                    if (unlikely(out->rs1 == 0))
                        return false;
                    out->rd = GPR_ZERO;
                    out->kind = instr_jalr;
                } else { /* C.MV */
                    out->rd = out->rs1;
                    out->rs1 = GPR_ZERO;
                    out->kind = instr_add;
                }
                return true;
            }
            unreachable();
            case 0x1: {
                *out = instr_crtype_read(raw);
                if (out->rs1 == 0 && out->rs2 == 0) { /* C.EBREAK */
                    fatal("unimplmented");
                } else if (out->rs2 == 0) { /* C.JALR */
                    out->rd = GPR_RA;
                    out->kind = instr_jalr;
                } else { /* C.ADD */
                    out->rd = out->rs1;
                    out->kind = instr_add;
                }
                return true;
            }
            unreachable();
            default: unreachable();
            }
        }
        unreachable();
        case 0x5: /* C.FSDSP */
            *out = instr_csstype_read(raw);
            out->rs1 = GPR_SP;
            out->kind = instr_fsd;
            return true;
        case 0x6: /* C.SWSP */
            *out = instr_csstype_read2(raw);
            out->rs1 = GPR_SP;
            out->kind = instr_sw;
            return true;
        case 0x7: /* C.SDSP */
            *out = instr_csstype_read(raw);
            out->rs1 = GPR_SP;
            out->kind = instr_sd;
            return true;
        default: fatal("unrecognized copcode");
        }
    }
    unreachable();
    case 0x3: {
        u32 opcode = OPCODE(raw);
        switch (opcode) {
        case 0x0: {
            u32 funct3 = FUNCT3(raw);

            *out = instr_itype_read(raw);
            switch (funct3) {
            case 0x0: /* LB */
                out->kind = instr_lb;
                return true;
            case 0x1: /* LH */
                out->kind = instr_lh;
                return true;
            case 0x2: /* LW */
                out->kind = instr_lw;
                return true;
            case 0x3: /* LD */
                out->kind = instr_ld;
                return true;
            case 0x4: /* LBU */
                out->kind = instr_lbu;
                return true;
            case 0x5: /* LHU */
                out->kind = instr_lhu;
                return true;
            case 0x6: /* LWU */
                out->kind = instr_lwu;
                return true;
            default: unreachable();
            }
        }
        unreachable();
        case 0x1: {
            u32 funct3 = FUNCT3(raw);

            *out = instr_itype_read(raw);
            switch (funct3) {
            case 0x2: /* FLW */
                out->kind = instr_flw;
                return true;
            case 0x3: /* FLD */
                out->kind = instr_fld;
                return true;
            default: unreachable();
            }
        }
        unreachable();
        case 0x3: {
            u32 funct3 = FUNCT3(raw);

            switch (funct3) {
            case 0x0: { /* FENCE */
                Instr _instr = {0};
                *out = _instr;
                out->kind = instr_fence;
                return true;
            }
            case 0x1: { /* FENCE.I */
                Instr _instr = {0};
                *out = _instr;
                out->kind = instr_fence_i;
                return true;
            }
            default: unreachable();
            }
        }
        unreachable();
        case 0x4: {
            u32 funct3 = FUNCT3(raw);

            *out = instr_itype_read(raw);
            switch (funct3) {
            case 0x0: /* ADDI */
                out->kind = instr_addi;
                return true;
            case 0x1: {
                u32 imm116 = IMM116(raw);
                if (imm116 == 0) { /* SLLI */
                    out->kind = instr_slli;
                } else {
                    unreachable();
                }
                return true;
            }
            unreachable();
            case 0x2: /* SLTI */
                out->kind = instr_slti;
                return true;
            case 0x3: /* SLTIU */
                out->kind = instr_sltiu;
                return true;
            case 0x4: /* XORI */
                out->kind = instr_xori;
                return true;
            case 0x5: {
                u32 imm116 = IMM116(raw);

                if (imm116 == 0x0) { /* SRLI */
                    out->kind = instr_srli;
                } else if (imm116 == 0x10) { /* SRAI */
                    out->kind = instr_srai;
                } else {
                    unreachable();
                }
                return true;
            }
            unreachable();
            case 0x6: /* ORI */
                out->kind = instr_ori;
                return true;
            case 0x7: /* ANDI */
                out->kind = instr_andi;
                return true;
            default:
                fatal("unrecognized funct3");
            }
        }
        unreachable();
        case 0x5: /* AUIPC */
            *out = instr_utype_read(raw);
            out->kind = instr_auipc;
            return true;
        case 0x6: {
            u32 funct3 = FUNCT3(raw);
            u32 funct7 = FUNCT7(raw);

            *out = instr_itype_read(raw);

            switch (funct3) {
            case 0x0: /* ADDIW */
                out->kind = instr_addiw;
                return true;
            case 0x1: /* SLLIW */
                if (unlikely(funct7 != 0))
                    return false;
                out->kind = instr_slliw;
                return true;
            case 0x5: {
                switch (funct7) {
                case 0x0: /* SRLIW */
                    out->kind = instr_srliw;
                    return true;
                case 0x20: /* SRAIW */
                    out->kind = instr_sraiw;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            default:
                fatal("unimplemented");
            }
        }
        unreachable();
        case 0x8: {
            u32 funct3 = FUNCT3(raw);

            *out = instr_stype_read(raw);
            switch (funct3) {
            case 0x0: /* SB */
                out->kind = instr_sb;
                return true;
            case 0x1: /* SH */
                out->kind = instr_sh;
                return true;
            case 0x2: /* SW */
                out->kind = instr_sw;
                return true;
            case 0x3: /* SD */
                out->kind = instr_sd;
                return true;
            default: unreachable();
            }
        }
        unreachable();
        case 0x9: {
            u32 funct3 = FUNCT3(raw);

            *out = instr_stype_read(raw);
            switch (funct3) {
            case 0x2: /* FSW */
                out->kind = instr_fsw;
                return true;
            case 0x3: /* FSD */
                out->kind = instr_fsd;
                return true;
            default: unreachable();
            }
        }
        unreachable();
        case 0xc: {
            *out = instr_rtype_read(raw);

            u32 funct3 = FUNCT3(raw);
            u32 funct7 = FUNCT7(raw);

            switch (funct7) {
            case 0x0: {
                switch (funct3) {
                case 0x0: /* ADD */
                    out->kind = instr_add;
                    return true;
                case 0x1: /* SLL */
                    out->kind = instr_sll;
                    return true;
                case 0x2: /* SLT */
                    out->kind = instr_slt;
                    return true;
                case 0x3: /* SLTU */
                    out->kind = instr_sltu;
                    return true;
                case 0x4: /* XOR */
                    out->kind = instr_xor;
                    return true;
                case 0x5: /* SRL */
                    out->kind = instr_srl;
                    return true;
                case 0x6: /* OR */
                    out->kind = instr_or;
                    return true;
                case 0x7: /* AND */
                    out->kind = instr_and;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x1: {
                switch (funct3) {
                case 0x0: /* MUL */
                    out->kind = instr_mul;
                    return true;
                case 0x1: /* MULH */
                    out->kind = instr_mulh;
                    return true;
                case 0x2: /* MULHSU */
                    out->kind = instr_mulhsu;
                    return true;
                case 0x3: /* MULHU */
                    out->kind = instr_mulhu;
                    return true;
                case 0x4: /* DIV */
                    out->kind = instr_div;
                    return true;
                case 0x5: /* DIVU */
                    out->kind = instr_divu;
                    return true;
                case 0x6: /* REM */
                    out->kind = instr_rem;
                    return true;
                case 0x7: /* REMU */
                    out->kind = instr_remu;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x20: {
                switch (funct3) {
                case 0x0: /* SUB */
                    out->kind = instr_sub;
                    return true;
                case 0x5: /* SRA */
                    out->kind = instr_sra;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            default: unreachable();
            }
        }
        unreachable();
        case 0xd: /* LUI */
            *out = instr_utype_read(raw);
            out->kind = instr_lui;
            return true;
        case 0xe: {
            *out = instr_rtype_read(raw);

            u32 funct3 = FUNCT3(raw);
            u32 funct7 = FUNCT7(raw);

            switch (funct7) {
            case 0x0: {
                switch (funct3) {
                case 0x0: /* ADDW */
                    out->kind = instr_addw;
                    return true;
                case 0x1: /* SLLW */
                    out->kind = instr_sllw;
                    return true;
                case 0x5: /* SRLW */
                    out->kind = instr_srlw;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x1: {
                switch (funct3) {
                case 0x0: /* MULW */
                    out->kind = instr_mulw;
                    return true;
                case 0x4: /* DIVW */
                    out->kind = instr_divw;
                    return true;
                case 0x5: /* DIVUW */
                    out->kind = instr_divuw;
                    return true;
                case 0x6: /* REMW */
                    out->kind = instr_remw;
                    return true;
                case 0x7: /* REMUW */
                    out->kind = instr_remuw;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x20: {
                switch (funct3) {
                case 0x0: /* SUBW */
                    out->kind = instr_subw;
                    return true;
                case 0x5: /* SRAW */
                    out->kind = instr_sraw;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            default: unreachable();
            }
        }
        unreachable();
        case 0x10: {
            u32 funct2 = FUNCT2(raw);

            *out = instr_fprtype_read(raw);
            switch (funct2) {
            case 0x0: /* FMADD.S */
                out->kind = instr_fmadd_s;
                return true;
            case 0x1: /* FMADD.D */
                out->kind = instr_fmadd_d;
                return true;
            default: unreachable();
            }
        }
        unreachable();
        case 0x11: {
            u32 funct2 = FUNCT2(raw);

            *out = instr_fprtype_read(raw);
            switch (funct2) {
            case 0x0: /* FMSUB.S */
                out->kind = instr_fmsub_s;
                return true;
            case 0x1: /* FMSUB.D */
                out->kind = instr_fmsub_d;
                return true;
            default: unreachable();
            }
        }
        unreachable();
        case 0x12: {
            u32 funct2 = FUNCT2(raw);

            *out = instr_fprtype_read(raw);
            switch (funct2) {
            case 0x0: /* FNMSUB.S */
                out->kind = instr_fnmsub_s;
                return true;
            case 0x1: /* FNMSUB.D */
                out->kind = instr_fnmsub_d;
                return true;
            default: unreachable();
            }
        }
        unreachable();
        case 0x13: {
            u32 funct2 = FUNCT2(raw);

            *out = instr_fprtype_read(raw);
            switch (funct2) {
            case 0x0: /* FNMADD.S */
                out->kind = instr_fnmadd_s;
                return true;
            case 0x1: /* FNMADD.D */
                out->kind = instr_fnmadd_d;
                return true;
            default: unreachable();
            }
        }
        unreachable();
        case 0x14: {
            u32 funct7 = FUNCT7(raw);

            *out = instr_rtype_read(raw);
            switch (funct7) {
            case 0x0:  /* FADD.S */
                out->kind = instr_fadd_s;
                return true;
            case 0x1:  /* FADD.D */
                out->kind = instr_fadd_d;
                return true;
            case 0x4:  /* FSUB.S */
                out->kind = instr_fsub_s;
                return true;
            case 0x5:  /* FSUB.D */
                out->kind = instr_fsub_d;
                return true;
            case 0x8:  /* FMUL.S */
                out->kind = instr_fmul_s;
                return true;
            case 0x9:  /* FMUL.D */
                out->kind = instr_fmul_d;
                return true;
            case 0xc:  /* FDIV.S */
                out->kind = instr_fdiv_s;
                return true;
            case 0xd:  /* FDIV.D */
                out->kind = instr_fdiv_d;
                return true;
            case 0x10: {
                u32 funct3 = FUNCT3(raw);

                switch (funct3) {
                case 0x0: /* FSGNJ.S */
                    out->kind = instr_fsgnj_s;
                    return true;
                case 0x1: /* FSGNJN.S */
                    out->kind = instr_fsgnjn_s;
                    return true;
                case 0x2: /* FSGNJX.S */
                    out->kind = instr_fsgnjx_s;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x11: {
                u32 funct3 = FUNCT3(raw);

                switch (funct3) {
                case 0x0: /* FSGNJ.D */
                    out->kind = instr_fsgnj_d;
                    return true;
                case 0x1: /* FSGNJN.D */
                    out->kind = instr_fsgnjn_d;
                    return true;
                case 0x2: /* FSGNJX.D */
                    out->kind = instr_fsgnjx_d;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x14: {
                u32 funct3 = FUNCT3(raw);

                switch (funct3) {
                case 0x0: /* FMIN.S */
                    out->kind = instr_fmin_s;
                    return true;
                case 0x1: /* FMAX.S */
                    out->kind = instr_fmax_s;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x15: {
                u32 funct3 = FUNCT3(raw);

                switch (funct3) {
                case 0x0: /* FMIN.D */
                    out->kind = instr_fmin_d;
                    return true;
                case 0x1: /* FMAX.D */
                    out->kind = instr_fmax_d;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x20: /* FCVT.S.D */
                if (unlikely(RS2(raw) != 1))
                    return false;
                out->kind = instr_fcvt_s_d;
                return true;
            case 0x21: /* FCVT.D.S */
                if (unlikely(RS2(raw) != 0))
                    return false;
                out->kind = instr_fcvt_d_s;
                return true;
            case 0x2c: /* FSQRT.S */
                if (unlikely(out->rs2 != 0))
                    return false;
                out->kind = instr_fsqrt_s;
                return true;
            case 0x2d: /* FSQRT.D */
                if (unlikely(out->rs2 != 0))
                    return false;
                out->kind = instr_fsqrt_d;
                return true;
            case 0x50: {
                u32 funct3 = FUNCT3(raw);

                switch (funct3) {
                case 0x0: /* FLE.S */
                    out->kind = instr_fle_s;
                    return true;
                case 0x1: /* FLT.S */
                    out->kind = instr_flt_s;
                    return true;
                case 0x2: /* FEQ.S */
                    out->kind = instr_feq_s;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x51: {
                u32 funct3 = FUNCT3(raw);

                switch (funct3) {
                case 0x0: /* FLE.D */
                    out->kind = instr_fle_d;
                    return true;
                case 0x1: /* FLT.D */
                    out->kind = instr_flt_d;
                    return true;
                case 0x2: /* FEQ.D */
                    out->kind = instr_feq_d;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x60: {
                u32 rs2 = RS2(raw);

                switch (rs2) {
                case 0x0: /* FCVT.W.S */
                    out->kind = instr_fcvt_w_s;
                    return true;
                case 0x1: /* FCVT.WU.S */
                    out->kind = instr_fcvt_wu_s;
                    return true;
                case 0x2: /* FCVT.L.S */
                    out->kind = instr_fcvt_l_s;
                    return true;
                case 0x3: /* FCVT.LU.S */
                    out->kind = instr_fcvt_lu_s;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x61: {
                u32 rs2 = RS2(raw);

                switch (rs2) {
                case 0x0: /* FCVT.W.D */
                    out->kind = instr_fcvt_w_d;
                    return true;
                case 0x1: /* FCVT.WU.D */
                    out->kind = instr_fcvt_wu_d;
                    return true;
                case 0x2: /* FCVT.L.D */
                    out->kind = instr_fcvt_l_d;
                    return true;
                case 0x3: /* FCVT.LU.D */
                    out->kind = instr_fcvt_lu_d;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x68: {
                u32 rs2 = RS2(raw);

                switch (rs2) {
                case 0x0: /* FCVT.S.W */
                    out->kind = instr_fcvt_s_w;
                    return true;
                case 0x1: /* FCVT.S.WU */
                    out->kind = instr_fcvt_s_wu;
                    return true;
                case 0x2: /* FCVT.S.L */
                    out->kind = instr_fcvt_s_l;
                    return true;
                case 0x3: /* FCVT.S.LU */
                    out->kind = instr_fcvt_s_lu;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x69: {
                u32 rs2 = RS2(raw);

                switch (rs2) {
                case 0x0: /* FCVT.D.W */
                    out->kind = instr_fcvt_d_w;
                    return true;
                case 0x1: /* FCVT.D.WU */
                    out->kind = instr_fcvt_d_wu;
                    return true;
                case 0x2: /* FCVT.D.L */
                    out->kind = instr_fcvt_d_l;
                    return true;
                case 0x3: /* FCVT.D.LU */
                    out->kind = instr_fcvt_d_lu;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x70: {
                if (unlikely(RS2(raw) != 0))
                    return false;
                u32 funct3 = FUNCT3(raw);

                switch (funct3) {
                case 0x0: /* FMV.X.W */
                    out->kind = instr_fmv_x_w;
                    return true;
                case 0x1: /* FCLASS.S */
                    out->kind = instr_fclass_s;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x71: {
                if (unlikely(RS2(raw) != 0))
                    return false;
                u32 funct3 = FUNCT3(raw);

                switch (funct3) {
                case 0x0: /* FMV.X.D */
                    out->kind = instr_fmv_x_d;
                    return true;
                case 0x1: /* FCLASS.D */
                    out->kind = instr_fclass_d;
                    return true;
                default: unreachable();
                }
            }
            unreachable();
            case 0x78: /* FMV_W_X */
                if (unlikely(RS2(raw) != 0 || FUNCT3(raw) != 0))
                    return false;
                out->kind = instr_fmv_w_x;
                return true;
            case 0x79: /* FMV_D_X */
                if (unlikely(RS2(raw) != 0 || FUNCT3(raw) != 0))
                    return false;
                out->kind = instr_fmv_d_x;
                return true;
            default: unreachable();
            }
        }
        unreachable();
        case 0x18: {
            *out = instr_btype_read(raw);

            u32 funct3 = FUNCT3(raw);
            switch (funct3) {
            case 0x0: /* BEQ */
                out->kind = instr_beq;
                out->cfc = true;
                return true;
            case 0x1: /* BNE */
                out->kind = instr_bne;
                out->cfc = true;
                return true;
            case 0x4: /* BLT */
                out->kind = instr_blt;
                out->cfc = true;
                return true;
            case 0x5: /* BGE */
                out->kind = instr_bge;
                out->cfc = true;
                return true;
            case 0x6: /* BLTU */
                out->kind = instr_bltu;
                out->cfc = true;
                return true;
            case 0x7: /* BGEU */
                out->kind = instr_bgeu;
                out->cfc = true;
                return true;
            default: unreachable();
            }
        }
        unreachable();
        case 0x19: /* JALR */
            *out = instr_itype_read(raw);
            out->kind = instr_jalr;
            out->cfc = true;
            return true;
        case 0x1b: /* JAL */
            *out = instr_jtype_read(raw);
            out->kind = instr_jal;
            out->cfc = true;
            return true;
        case 0x1c: {
            if (raw == 0x73) { /* ECALL */
                out->kind = instr_ecall;
                out->cfc = true;
                return true;
            }

            u32 funct3 = FUNCT3(raw);
            *out = instr_csrtype_read(raw);
            switch(funct3) {
            case 0x1: /* CSRRW */
                out->kind = instr_csrrw;
                return true;
            case 0x2: /* CSRRS */
                out->kind = instr_csrrs;
                return true;
            case 0x3: /* CSRRC */
                out->kind = instr_csrrc;
                return true;
            case 0x5: /* CSRRWI */
                out->kind = instr_csrrwi;
                return true;
            case 0x6: /* CSRRSI */
                out->kind = instr_csrrsi;
                return true;
            case 0x7: /* CSRRCI */
                out->kind = instr_csrrci;
                return true;
            default: unreachable();
            }
        }
        unreachable();
        default: unreachable();
        }
    }
    unreachable();
    default: unreachable();
    }
}
