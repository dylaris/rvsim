#include "decoder.h"

#include <string.h>

#define QUADRANT(data)   (((data) >>  0) & 0x3)

#define OPCODE(data)     (((data) >>  0) & 0x7f)
#define RD(data)         (((data) >>  7) & 0x1f)
#define RS1(data)        (((data) >> 15) & 0x1f)
#define RS2(data)        (((data) >> 20) & 0x1f)
#define RS3(data)        (((data) >> 27) & 0x1f)
#define FUNCT2(data)     (((data) >> 25) & 0x3 )
#define FUNCT3(data)     (((data) >> 12) & 0x7 )
#define FUNCT5HIGH(data) (((data) >> 27) & 0x1f)
#define FUNCT5LOW(data)  (((data) >> 20) & 0x1f)
#define FUNCT7(data)     (((data) >> 25) & 0x7f)
#define FUNCT12(data)    (((data) >> 20) & 0xfff)

#define COPCODE(data)     (((data) >>  0)  & 0x3)
#define CRD(data)         (((data) >>  7)  & 0x1f)
#define CRS1(data)        (((data) >>  7)  & 0x1f)
#define CRS2(data)        (((data) >>  2)  & 0x1f)
#define CRP(data,h,l)     (((data) >> (l)) & 0x7)
#define CFUNCT1(data)     (((data) >> 12)  & 0x1)
#define CFUNCT2HIGH(data) (((data) >> 10)  & 0x3)
#define CFUNCT2LOW(data)  (((data) >>  5)  & 0x3)
#define CFUNCT3(data)     (((data) >> 13)  & 0x7)
#define CFUNCT5HIGH(data) (((data) >>  7)  & 0x1f)
#define CFUNCT5LOW(data)  (((data) >>  2)  & 0x1f)

static inline i32 inst__sign_extend(i32 x, u32 n)
{
    return (i32) ((x << (32 - n)) >> (32 - n));
}


static inline Inst inst__decode_blank(const InstDef *defp, u32 data)
{
    (void) data;
    return (Inst) {
        .kind = defp->kind,
    };
}

static inline Inst inst__decode_crtype(const InstDef *defp, u32 data)
{
    Inst inst = {
        .kind = defp->kind,
        .rd   = CRD(data),
        .rs1  = CRS1(data),
        .rs2  = CRS2(data),
        .rvc  = true,
    };
    if (strcmp(defp->name, "c.jr") == 0)
        inst.rd = RI_ZERO;
    else if (strcmp(defp->name, "c.mv") == 0)
        inst.rs1 = RI_ZERO;
    else if (strcmp(defp->name, "c.jalr") == 0)
        inst.rd = RI_RA;
    return inst;
}

static inline Inst inst__decode_citype(const InstDef *defp, u32 data)
{
    u32 imm4_0 = (u32) ((data >>  2) & 0x1f);
    u32 imm5   = (u32) ((data >> 12) & 0x1);
    i32 imm    = (i32) ((imm5 << 5) | imm4_0);
    Inst inst = {
        .kind = defp->kind,
        .rd   = CRD(data),
        .rs1  = CRS1(data),
        .imm  = inst__sign_extend(imm, 6),
        .rvc  = true,
    };
    if (strcmp(defp->name, "c.li") == 0)
        inst.rs1 = RI_ZERO;
    return inst;
}

static inline Inst inst__decode_citype2(const InstDef *defp, u32 data)
{
    u32 imm8_6 = (u32) ((data >>  2) & 0x7);
    u32 imm4_3 = (u32) ((data >>  5) & 0x3);
    u32 imm5   = (u32) ((data >> 12) & 0x1);
    i32 imm    = (i32) ((imm8_6 << 6) | (imm4_3 << 3) | (imm5 << 5));
    Inst inst = {
        .kind = defp->kind,
        .rd   = CRD(data),
        .rs1  = CRS1(data),
        .imm  = imm,
        .rvc  = true,
    };
    if (strcmp(defp->name, "c.fldsp") == 0 || strcmp(defp->name, "c.ldsp") == 0)
        inst.rs1 = RI_SP;
    return inst;
}

static inline Inst inst__decode_citype3(const InstDef *defp, u32 data)
{
    u32 imm5   = (u32) ((data >>  2) & 0x1);
    u32 imm8_7 = (u32) ((data >>  3) & 0x3);
    u32 imm6   = (u32) ((data >>  5) & 0x1);
    u32 imm4   = (u32) ((data >>  6) & 0x1);
    u32 imm9   = (u32) ((data >> 12) & 0x1);
    i32 imm    = (i32) ((imm5 << 5) | (imm8_7 << 7) | (imm6 << 6) | (imm4 << 4) | (imm9 << 9));
    return (Inst) {
        .kind = defp->kind,
        .rd   = CRD(data),
        .rs1  = CRS1(data),
        .imm  = inst__sign_extend(imm, 10),
        .rvc  = true,
    };
}

static inline Inst inst__decode_citype4(const InstDef *defp, u32 data)
{
    u32 imm5   = (u32) ((data >> 12) & 0x1);
    u32 imm4_2 = (u32) ((data >>  4) & 0x7);
    u32 imm7_6 = (u32) ((data >>  2) & 0x3);
    i32 imm    = (i32) ((imm5 << 5) | (imm4_2 << 2) | (imm7_6 << 6));
    Inst inst = {
        .kind = defp->kind,
        .rd   = CRD(data),
        .rs1  = CRS1(data),
        .imm  = imm,
        .rvc  = true,
    };
    if (strcmp(defp->name, "c.lwsp") == 0)
        inst.rs1 = RI_SP;
    return inst;
}

static inline Inst inst__decode_citype5(const InstDef *defp, u32 data)
{
    u32 imm16_12 = (u32) ((data >>  2) & 0x1f);
    u32 imm17    = (u32) ((data >> 12) & 0x1);
    i32 imm      = (i32) ((imm16_12 << 12) | (imm17 << 17));
    return (Inst) {
        .kind = defp->kind,
        .rd   = CRD(data),
        .rs1  = CRS1(data),
        .imm  = inst__sign_extend(imm, 18),
        .rvc  = true,
    };
}

static inline Inst inst__decode_csstype(const InstDef *defp, u32 data)
{
    u32 imm8_6 = (u32) ((data >>  7) & 0x7);
    u32 imm5_3 = (u32) ((data >> 10) & 0x7);
    i32 imm    = (i32) ((imm8_6 << 6) | (imm5_3 << 3));
    Inst inst = {
        .kind = defp->kind,
        .rs2  = CRS2(data),
        .imm  = imm,
        .rvc  = true,
    };
    if (strcmp(defp->name, "c.fsdsp") == 0 || strcmp(defp->name, "c.sdsp") == 0)
        inst.rs1 = RI_SP;
    return inst;
}

static inline Inst inst__decode_csstype2(const InstDef *defp, u32 data)
{
    u32 imm7_6 = (u32) ((data >> 7) & 0x3);
    u32 imm5_2 = (u32) ((data >> 9) & 0xf);
    i32 imm    = (i32) ((imm7_6 << 6) | (imm5_2 << 2));
    Inst inst = {
        .kind = defp->kind,
        .rs2  = CRS2(data),
        .imm  = imm,
        .rvc  = true,
    };
    if (strcmp(defp->name, "c.swsp") == 0)
        inst.rs1 = RI_SP;
    return inst;
}

static inline Inst inst__decode_ciwtype(const InstDef *defp, u32 data)
{
    u32 imm3   = (u32) ((data >>  5) & 0x1);
    u32 imm2   = (u32) ((data >>  6) & 0x1);
    u32 imm9_6 = (u32) ((data >>  7) & 0xf);
    u32 imm5_4 = (u32) ((data >> 11) & 0x3);
    i32 imm    = (i32) ((imm3 << 3) | (imm2 << 2) | (imm9_6 << 6) | (imm5_4 << 4));
    Inst inst = {
        .kind = defp->kind,
        .imm  = imm,
        .rd   = CRP(data,4,2) + 8,
        .rvc  = true,
    };
    if (strcmp(defp->name, "c.addi4spn") == 0)
        inst.rs1 = RI_SP;
    return inst;
}

static inline Inst inst__decode_cltype(const InstDef *defp, u32 data)
{
    u32 imm6   = (u32) ((data >>  5) & 0x1);
    u32 imm2   = (u32) ((data >>  6) & 0x1);
    u32 imm5_3 = (u32) ((data >> 10) & 0x7);
    i32 imm    = (i32) ((imm6 << 6) | (imm2 << 2) | (imm5_3 << 3));
    return (Inst) {
        .kind = defp->kind,
        .rd   = CRP(data,4,2) + 8,
        .rs1  = CRP(data,9,7) + 8,
        .imm  = imm,
        .rvc  = true,
    };
}

static inline Inst inst__decode_cltype2(const InstDef *defp, u32 data)
{
    u32 imm7_6 = (u32) ((data >>  5) & 0x3);
    u32 imm5_3 = (u32) ((data >> 10) & 0x7);
    i32 imm    = (i32) ((imm7_6 << 6) | (imm5_3 << 3));
    return (Inst) {
        .kind = defp->kind,
        .rd   = CRP(data,4,2) + 8,
        .rs1  = CRP(data,9,7) + 8,
        .imm  = imm,
        .rvc  = true,
    };
}

static inline Inst inst__decode_cstype(const InstDef *defp, u32 data)
{
    u32 imm7_6 = (u32) ((data >>  5) & 0x3);
    u32 imm5_3 = (u32) ((data >> 10) & 0x7);
    i32 imm    = (i32) (((imm7_6 << 6) | (imm5_3 << 3)));
    return (Inst) {
        .kind = defp->kind,
        .rs1  = CRP(data,9,7) + 8,
        .rs2  = CRP(data,4,2) + 8,
        .imm  = imm,
        .rvc  = true,
    };
}

static inline Inst inst__decode_cstype2(const InstDef *defp, u32 data)
{
    u32 imm6   = (u32) ((data >>  5) & 0x1);
    u32 imm2   = (u32) ((data >>  6) & 0x1);
    u32 imm5_3 = (u32) ((data >> 10) & 0x7);
    i32 imm    = (i32) (((imm6 << 6) | (imm2 << 2) | (imm5_3 << 3)));
    return (Inst) {
        .kind = defp->kind,
        .rs1  = CRP(data,9,7) + 8,
        .rs2  = CRP(data,4,2) + 8,
        .imm  = imm,
        .rvc  = true,
    };
}

static inline Inst inst__decode_catype(const InstDef *defp, u32 data)
{
    return (Inst) {
        .kind = defp->kind,
        .rd   = CRP(data,9,7) + 8,
        .rs1  = CRP(data,9,7) + 8,
        .rs2  = CRP(data,4,2) + 8,
        .rvc  = true,
    };
}

static inline Inst inst__decode_cbtype(const InstDef *defp, u32 data)
{
    u32 imm5   = (u32) ((data >>  2) & 0x1);
    u32 imm2_1 = (u32) ((data >>  3) & 0x3);
    u32 imm7_6 = (u32) ((data >>  5) & 0x3);
    u32 imm4_3 = (u32) ((data >> 10) & 0x3);
    u32 imm8   = (u32) ((data >> 12) & 0x1);
    i32 imm    = (i32) ((imm8 << 8) | (imm7_6 << 6) | (imm5 << 5) | (imm4_3 << 3) | (imm2_1 << 1));
    Inst inst = {
        .kind = defp->kind,
        .rd   = CRP(data,9,7) + 8,
        .rs1  = CRP(data,9,7) + 8,
        .imm  = inst__sign_extend(imm, 9),
        .rvc  = true,
    };
    if (strcmp(defp->name, "c.beqz") == 0 || strcmp(defp->name, "c.bnez") == 0)
        inst.rs2 = RI_ZERO;
    return inst;
}

static inline Inst inst__decode_cbtype2(const InstDef *defp, u32 data)
{
    u32 imm4_0 = (u32) ((data >>  2) & 0x1f);
    u32 imm5   = (u32) ((data >> 12) & 0x1);
    i32 imm    = (i32) ((imm5 << 5) | imm4_0);
    return (Inst) {
        .kind = defp->kind,
        .rd   = CRP(data,9,7) + 8,
        .rs1  = CRP(data,9,7) + 8,
        .imm  = inst__sign_extend(imm, 6),
        .rvc  = true,
    };
}

static inline Inst inst__decode_cjtype(const InstDef *defp, u32 data)
{
    u32 imm5   = (u32) ((data >>  2) & 0x1);
    u32 imm3_1 = (u32) ((data >>  3) & 0x7);
    u32 imm7   = (u32) ((data >>  6) & 0x1);
    u32 imm6   = (u32) ((data >>  7) & 0x1);
    u32 imm10  = (u32) ((data >>  8) & 0x1);
    u32 imm9_8 = (u32) ((data >>  9) & 0x3);
    u32 imm4   = (u32) ((data >> 11) & 0x1);
    u32 imm11  = (u32) ((data >> 12) & 0x1);
    i32 imm    = (i32) (((imm5  << 5)  | (imm3_1 << 1) | (imm7 << 7) | (imm6 << 6) |
                         (imm10 << 10) | (imm9_8 << 8) | (imm4 << 4) | (imm11 << 11)));
    Inst inst = {
        .kind = defp->kind,
        .imm  = inst__sign_extend(imm, 12),
        .rvc  = true,
    };
    if (strcmp(defp->name, "c.j") == 0)
        inst.rd = RI_ZERO;
    return inst;
}

static inline Inst inst__decode_rtype(const InstDef *defp, u32 data)
{
    return (Inst) {
        .kind = defp->kind,
        .rd   = RD(data),
        .rs1  = RS1(data),
        .rs2  = RS2(data),
    };
}

static inline Inst inst__decode_r4type(const InstDef *defp, u32 data)
{
    return (Inst) {
        .kind = defp->kind,
        .rd   = RD(data),
        .rs1  = RS1(data),
        .rs2  = RS2(data),
        .rs3  = RS3(data),
    };
}

static inline Inst inst__decode_itype(const InstDef *defp, u32 data)
{
    i32 imm = (i32) ((data >> 20) & 0xfff);
    return (Inst) {
        .kind = defp->kind,
        .rd   = RD(data),
        .rs1  = RS1(data),
        .imm  = inst__sign_extend(imm, 12)
    };
}

static inline Inst inst__decode_stype(const InstDef *defp, u32 data)
{
    u32 imm4_0  = (u32) ((data >>  7) & 0x1f);
    u32 imm11_5 = (u32) ((data >> 25) & 0x7f);
    i32 imm     = (i32) ((imm11_5 << 5) | imm4_0);
    return (Inst) {
        .kind = defp->kind,
        .rs1  = RS1(data),
        .rs2  = RS2(data),
        .imm  = inst__sign_extend(imm, 12)
    };
}

static inline Inst inst__decode_btype(const InstDef *defp, u32 data)
{
    u32 imm11   = (u32) ((data >>  7) & 0x1);
    u32 imm4_1  = (u32) ((data >>  8) & 0xf);
    u32 imm10_5 = (u32) ((data >> 25) & 0x3f);
    u32 imm12   = (u32) ((data >> 31) & 0x1);
    i32 imm     = (i32) ((imm12 << 12) | (imm11 << 11) | (imm10_5 << 5) | (imm4_1 << 1));
    return (Inst) {
        .kind = defp->kind,
        .rs1  = RS1(data),
        .rs2  = RS2(data),
        .imm  = inst__sign_extend(imm, 13)
    };
}

static inline Inst inst__decode_utype(const InstDef *defp, u32 data)
{
    return (Inst) {
        .kind = defp->kind,
        .rd   = RD(data),
        .imm  = (i32) (data & 0xfffff000)
    };
}

static inline Inst inst__decode_jtype(const InstDef *defp, u32 data)
{
    u32 imm19_12 = (u32) ((data >> 12) & 0xff);
    u32 imm11    = (u32) ((data >> 20) & 0x1);
    u32 imm10_1  = (u32) ((data >> 21) & 0x3ff);
    u32 imm20    = (u32) ((data >> 31) & 0x1);
    i32 imm      = (i32) ((imm20 << 20) | (imm19_12 << 12) | (imm11 << 11) | (imm10_1 << 1));
    return (Inst) {
        .kind = defp->kind,
        .rd   = RD(data),
        .imm  = inst__sign_extend(imm, 21)
    };
}

#define IGNORE 0xffff
#define IGNORE_RVC  IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE
#define IGNORE_NRVC IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE
static InstDef inst_table[] = {
    /* name        kind          opc   f2      f3      f5h     f5l     f7      f12     ...         decode            */
    { "lb",        IK_lb,        0x03, IGNORE, 0x0,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "lh",        IK_lh,        0x03, IGNORE, 0x1,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "lw",        IK_lw,        0x03, IGNORE, 0x2,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "ld",        IK_ld,        0x03, IGNORE, 0x3,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "lbu",       IK_lbu,       0x03, IGNORE, 0x4,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "lhu",       IK_lhu,       0x03, IGNORE, 0x5,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "lwu",       IK_lwu,       0x03, IGNORE, 0x6,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "flw",       IK_flw,       0x07, IGNORE, 0x2,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "fld",       IK_fld,       0x07, IGNORE, 0x3,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "fence",     IK_fence,     0x0f, IGNORE, 0x0,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_blank },
    { "fence.i",   IK_fence_i,   0x0f, IGNORE, 0x1,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_blank },
    { "addi",      IK_addi,      0x13, IGNORE, 0x0,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "slli",      IK_slli,      0x13, IGNORE, 0x1,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_itype },
    { "slti",      IK_slti,      0x13, IGNORE, 0x2,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "sltiu",     IK_sltiu,     0x13, IGNORE, 0x3,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "xori",      IK_xori,      0x13, IGNORE, 0x4,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "srai",      IK_srai,      0x13, IGNORE, 0x5,    IGNORE, IGNORE, 0x20,   IGNORE, IGNORE_RVC, inst__decode_itype },
    { "srli",      IK_srli,      0x13, IGNORE, 0x5,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_itype },
    { "ori",       IK_ori,       0x13, IGNORE, 0x6,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "andi",      IK_andi,      0x13, IGNORE, 0x7,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "auipc",     IK_auipc,     0x17, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_utype },
    { "addiw",     IK_addiw,     0x1b, IGNORE, 0x0,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "slliw",     IK_slliw,     0x1b, IGNORE, 0x1,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_itype },
    { "srliw",     IK_srliw,     0x1b, IGNORE, 0x5,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_itype },
    { "sraiw",     IK_sraiw,     0x1b, IGNORE, 0x5,    IGNORE, IGNORE, 0x20,   IGNORE, IGNORE_RVC, inst__decode_itype },
    { "sb",        IK_sb,        0x23, IGNORE, 0x0,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_stype },
    { "sh",        IK_sh,        0x23, IGNORE, 0x1,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_stype },
    { "sw",        IK_sw,        0x23, IGNORE, 0x2,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_stype },
    { "sd",        IK_sd,        0x23, IGNORE, 0x3,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_stype },
    { "fsw",       IK_fsw,       0x27, IGNORE, 0x2,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_stype },
    { "fsd",       IK_fsd,       0x27, IGNORE, 0x3,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_stype },
    // { "amoadd.w",  ik_amoadd_w,  0x2f, IGNORE, 0x2,    0x00,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amoswap.w", ik_amoswap_W, 0x2f, IGNORE, 0x2,    0x01,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "lr.w",      ik_lr_w,      0x2f, IGNORE, 0x2,    0x02,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "sc.w",      ik_sc_w,      0x2f, IGNORE, 0x2,    0x03,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amoxor.w",  ik_amoxor_w,  0x2f, IGNORE, 0x2,    0x04,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amoor.w",   ik_amoor_w,   0x2f, IGNORE, 0x2,    0x08,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amoand.w",  ik_amoand_w,  0x2f, IGNORE, 0x2,    0x0c,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amomin.w",  ik_amomin_w,  0x2f, IGNORE, 0x2,    0x10,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amomax.w",  ik_amomax_w,  0x2f, IGNORE, 0x2,    0x14,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amominu.w", ik_amominu_W, 0x2f, IGNORE, 0x2,    0x18,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amomaxu.w", ik_amomaxu_W, 0x2f, IGNORE, 0x2,    0x1c,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amoadd.d",  ik_amoadd_d,  0x2f, IGNORE, 0x3,    0x00,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amoswap.d", ik_amoswap_D, 0x2f, IGNORE, 0x3,    0x01,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "lr.d",      ik_lr_d,      0x2f, IGNORE, 0x3,    0x02,   0x00,   IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "sc.d",      ik_sc_d,      0x2f, IGNORE, 0x3,    0x03,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amoxor.d",  ik_amoxor_d,  0x2f, IGNORE, 0x3,    0x04,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amoor.d",   ik_amoor_d,   0x2f, IGNORE, 0x3,    0x08,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amoand.d",  ik_amoand_d,  0x2f, IGNORE, 0x3,    0x0c,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amomin.d",  ik_amomin_d,  0x2f, IGNORE, 0x3,    0x10,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amomax.d",  ik_amomax_d,  0x2f, IGNORE, 0x3,    0x14,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amominu.d", ik_amominu_D, 0x2f, IGNORE, 0x3,    0x18,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    // { "amomaxu.d", ik_amomaxu_D, 0x2f, IGNORE, 0x3,    0x1c,   IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "add",       IK_add,       0x33, IGNORE, 0x0,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "mul",       IK_mul,       0x33, IGNORE, 0x0,    IGNORE, IGNORE, 0x01,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "sub",       IK_sub,       0x33, IGNORE, 0x0,    IGNORE, IGNORE, 0x20,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "sll",       IK_sll,       0x33, IGNORE, 0x1,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "mulh",      IK_mulh,      0x33, IGNORE, 0x1,    IGNORE, IGNORE, 0x01,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "slt",       IK_slt,       0x33, IGNORE, 0x2,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "mulhsu",    IK_mulhsu,    0x33, IGNORE, 0x2,    IGNORE, IGNORE, 0x01,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "sltu",      IK_sltu,      0x33, IGNORE, 0x3,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "mulhu",     IK_mulhu,     0x33, IGNORE, 0x3,    IGNORE, IGNORE, 0x01,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "xor",       IK_xor,       0x33, IGNORE, 0x4,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "div",       IK_div,       0x33, IGNORE, 0x4,    IGNORE, IGNORE, 0x01,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "srl",       IK_srl,       0x33, IGNORE, 0x5,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "divu",      IK_divu,      0x33, IGNORE, 0x5,    IGNORE, IGNORE, 0x01,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "sra",       IK_sra,       0x33, IGNORE, 0x5,    IGNORE, IGNORE, 0x20,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "or",        IK_or,        0x33, IGNORE, 0x6,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "rem",       IK_rem,       0x33, IGNORE, 0x6,    IGNORE, IGNORE, 0x01,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "and",       IK_and,       0x33, IGNORE, 0x7,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "remu",      IK_remu,      0x33, IGNORE, 0x7,    IGNORE, IGNORE, 0x01,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "lui",       IK_lui,       0x37, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_utype },
    { "addw",      IK_addw,      0x3b, IGNORE, 0x0,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "mulw",      IK_mulw,      0x3b, IGNORE, 0x0,    IGNORE, IGNORE, 0x01,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "subw",      IK_subw,      0x3b, IGNORE, 0x0,    IGNORE, IGNORE, 0x20,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "sllw",      IK_sllw,      0x3b, IGNORE, 0x1,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "divw",      IK_divw,      0x3b, IGNORE, 0x4,    IGNORE, IGNORE, 0x01,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "srlw",      IK_srlw,      0x3b, IGNORE, 0x5,    IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "divuw",     IK_divuw,     0x3b, IGNORE, 0x5,    IGNORE, IGNORE, 0x01,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "sraw",      IK_sraw,      0x3b, IGNORE, 0x5,    IGNORE, IGNORE, 0x20,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "remw",      IK_remw,      0x3b, IGNORE, 0x6,    IGNORE, IGNORE, 0x01,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "remuw",     IK_remuw,     0x3b, IGNORE, 0x7,    IGNORE, IGNORE, 0x01,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fmadd.s",   IK_fmadd_s,   0x43, 0x0,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_r4type },
    { "fmadd.d",   IK_fmadd_d,   0x43, 0x1,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_r4type },
    { "fmsub.s",   IK_fmsub_s,   0x47, 0x0,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_r4type },
    { "fmsub.d",   IK_fmsub_d,   0x47, 0x1,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_r4type },
    { "fnmsub.s",  IK_fnmsub_s,  0x4b, 0x0,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_r4type },
    { "fnmsub.d",  IK_fnmsub_d,  0x4b, 0x1,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_r4type },
    { "fnmadd.s",  IK_fnmadd_s,  0x4f, 0x0,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_r4type },
    { "fnmadd.d",  IK_fnmadd_d,  0x4f, 0x1,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_r4type },
    { "fcvt.d.wu", IK_fcvt_d_wu, 0x53, IGNORE, 0x0,    IGNORE, 0x01,   0x69,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fmv.x.w",   IK_fmv_x_w,   0x53, IGNORE, 0x0,    IGNORE, 0x00,   0x70,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fmv.x.d",   IK_fmv_x_d,   0x53, IGNORE, 0x0,    IGNORE, 0x00,   0x71,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fmv.w.x",   IK_fmv_w_x,   0x53, IGNORE, 0x0,    IGNORE, 0x00,   0x78,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fmv.d.x",   IK_fmv_d_x,   0x53, IGNORE, 0x0,    IGNORE, 0x00,   0x79,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fsgnj.s",   IK_fsgnj_s,   0x53, IGNORE, 0x0,    IGNORE, IGNORE, 0x10,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fsgnj.d",   IK_fsgnj_d,   0x53, IGNORE, 0x0,    IGNORE, IGNORE, 0x11,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fmin.s",    IK_fmin_s,    0x53, IGNORE, 0x0,    IGNORE, IGNORE, 0x14,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fmin.d",    IK_fmin_d,    0x53, IGNORE, 0x0,    IGNORE, IGNORE, 0x15,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fle.s",     IK_fle_s,     0x53, IGNORE, 0x0,    IGNORE, IGNORE, 0x50,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fle.d",     IK_fle_d,     0x53, IGNORE, 0x0,    IGNORE, IGNORE, 0x51,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fclass.s",  IK_fclass_s,  0x53, IGNORE, 0x1,    IGNORE, 0x00,   0x70,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fclass.d",  IK_fclass_d,  0x53, IGNORE, 0x1,    IGNORE, 0x00,   0x71,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fsgnjn.s",  IK_fsgnjn_s,  0x53, IGNORE, 0x1,    IGNORE, IGNORE, 0x10,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fsgnjn.d",  IK_fsgnjn_d,  0x53, IGNORE, 0x1,    IGNORE, IGNORE, 0x11,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fmax.s",    IK_fmax_s,    0x53, IGNORE, 0x1,    IGNORE, IGNORE, 0x14,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fmax.d",    IK_fmax_d,    0x53, IGNORE, 0x1,    IGNORE, IGNORE, 0x15,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "flt.s",     IK_flt_s,     0x53, IGNORE, 0x1,    IGNORE, IGNORE, 0x50,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "flt.d",     IK_flt_d,     0x53, IGNORE, 0x1,    IGNORE, IGNORE, 0x51,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fsgnjx.s",  IK_fsgnjx_s,  0x53, IGNORE, 0x2,    IGNORE, IGNORE, 0x10,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fsgnjx.d",  IK_fsgnjx_d,  0x53, IGNORE, 0x2,    IGNORE, IGNORE, 0x11,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "feq.s",     IK_feq_s,     0x53, IGNORE, 0x2,    IGNORE, IGNORE, 0x50,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "feq.d",     IK_feq_d,     0x53, IGNORE, 0x2,    IGNORE, IGNORE, 0x51,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.d.s",  IK_fcvt_d_s,  0x53, IGNORE, IGNORE, IGNORE, 0x00,   0x21,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fsqrt.s",   IK_fsqrt_s,   0x53, IGNORE, IGNORE, IGNORE, 0x00,   0x2c,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fsqrt.d",   IK_fsqrt_d,   0x53, IGNORE, IGNORE, IGNORE, 0x00,   0x2d,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.w.s",  IK_fcvt_w_s,  0x53, IGNORE, IGNORE, IGNORE, 0x00,   0x60,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.w.d",  IK_fcvt_w_d,  0x53, IGNORE, IGNORE, IGNORE, 0x00,   0x61,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.s.w",  IK_fcvt_s_w,  0x53, IGNORE, IGNORE, IGNORE, 0x00,   0x68,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.d.w",  IK_fcvt_d_w,  0x53, IGNORE, IGNORE, IGNORE, 0x00,   0x69,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.s.d",  IK_fcvt_s_d,  0x53, IGNORE, IGNORE, IGNORE, 0x01,   0x20,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.wu.s", IK_fcvt_wu_s, 0x53, IGNORE, IGNORE, IGNORE, 0x01,   0x60,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.wu.d", IK_fcvt_wu_d, 0x53, IGNORE, IGNORE, IGNORE, 0x01,   0x61,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.s.wu", IK_fcvt_s_wu, 0x53, IGNORE, IGNORE, IGNORE, 0x01,   0x68,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.l.s",  IK_fcvt_l_s,  0x53, IGNORE, IGNORE, IGNORE, 0x02,   0x60,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.l.d",  IK_fcvt_l_d,  0x53, IGNORE, IGNORE, IGNORE, 0x02,   0x61,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.s.l",  IK_fcvt_s_l,  0x53, IGNORE, IGNORE, IGNORE, 0x02,   0x68,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.d.l",  IK_fcvt_d_l,  0x53, IGNORE, IGNORE, IGNORE, 0x02,   0x69,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.lu.s", IK_fcvt_lu_s, 0x53, IGNORE, IGNORE, IGNORE, 0x03,   0x60,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.lu.d", IK_fcvt_lu_d, 0x53, IGNORE, IGNORE, IGNORE, 0x03,   0x61,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.s.lu", IK_fcvt_s_lu, 0x53, IGNORE, IGNORE, IGNORE, 0x03,   0x68,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fcvt.d.lu", IK_fcvt_d_lu, 0x53, IGNORE, IGNORE, IGNORE, 0x03,   0x69,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fadd.s",    IK_fadd_s,    0x53, IGNORE, IGNORE, IGNORE, IGNORE, 0x00,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fadd.d",    IK_fadd_d,    0x53, IGNORE, IGNORE, IGNORE, IGNORE, 0x01,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fsub.s",    IK_fsub_s,    0x53, IGNORE, IGNORE, IGNORE, IGNORE, 0x04,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fsub.d",    IK_fsub_d,    0x53, IGNORE, IGNORE, IGNORE, IGNORE, 0x05,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fmul.s",    IK_fmul_s,    0x53, IGNORE, IGNORE, IGNORE, IGNORE, 0x08,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fmul.d",    IK_fmul_d,    0x53, IGNORE, IGNORE, IGNORE, IGNORE, 0x09,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fdiv.s",    IK_fdiv_s,    0x53, IGNORE, IGNORE, IGNORE, IGNORE, 0x0c,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "fdiv.d",    IK_fdiv_d,    0x53, IGNORE, IGNORE, IGNORE, IGNORE, 0x0d,   IGNORE, IGNORE_RVC, inst__decode_rtype },
    { "beq",       IK_beq,       0x63, IGNORE, 0x0,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_btype },
    { "bne",       IK_bne,       0x63, IGNORE, 0x1,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_btype },
    { "blt",       IK_blt,       0x63, IGNORE, 0x4,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_btype },
    { "bge",       IK_bge,       0x63, IGNORE, 0x5,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_btype },
    { "bltu",      IK_bltu,      0x63, IGNORE, 0x6,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_btype },
    { "bgeu",      IK_bgeu,      0x63, IGNORE, 0x7,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_btype },
    { "jalr",      IK_jalr,      0x67, IGNORE, 0x0,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "jal",       IK_jal,       0x6f, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_jtype },
    { "ebreak",    IK_ebreak,    0x73, IGNORE, 0x0,    IGNORE, IGNORE, IGNORE, 0x001,  IGNORE_RVC, inst__decode_itype },
    { "ecall",     IK_ecall,     0x73, IGNORE, 0x0,    IGNORE, IGNORE, IGNORE, 0x000,  IGNORE_RVC, inst__decode_itype },
    { "csrrw",     IK_csrrw,     0x73, IGNORE, 0x1,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "csrrs",     IK_csrrs,     0x73, IGNORE, 0x2,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "csrrc",     IK_csrrc,     0x73, IGNORE, 0x3,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "csrrwi",    IK_csrrwi,    0x73, IGNORE, 0x5,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "csrrsi",    IK_csrrsi,    0x73, IGNORE, 0x6,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
    { "csrrci",    IK_csrrci,    0x73, IGNORE, 0x7,    IGNORE, IGNORE, IGNORE, IGNORE, IGNORE_RVC, inst__decode_itype },
#define RVC_START 134
    /* name         kind       ...          copc  cf1     cf2h    cf2l    cf3  cf5h,   cf5l    decode              */
    { "c.addi4spn", IK_addi,   IGNORE_NRVC, 0x0,  IGNORE, IGNORE, IGNORE, 0x0, IGNORE, IGNORE, inst__decode_ciwtype },
    { "c.fld",      IK_fld,    IGNORE_NRVC, 0x0,  IGNORE, IGNORE, IGNORE, 0x1, IGNORE, IGNORE, inst__decode_cltype2 },
    { "c.lw",       IK_lw,     IGNORE_NRVC, 0x0,  IGNORE, IGNORE, IGNORE, 0x2, IGNORE, IGNORE, inst__decode_cltype },
    { "c.ld",       IK_ld,     IGNORE_NRVC, 0x0,  IGNORE, IGNORE, IGNORE, 0x3, IGNORE, IGNORE, inst__decode_cltype2 },
    { "c.fsd",      IK_fsd,    IGNORE_NRVC, 0x0,  IGNORE, IGNORE, IGNORE, 0x5, IGNORE, IGNORE, inst__decode_cstype },
    { "c.sw",       IK_sw,     IGNORE_NRVC, 0x0,  IGNORE, IGNORE, IGNORE, 0x6, IGNORE, IGNORE, inst__decode_cstype2 },
    { "c.sd",       IK_sd,     IGNORE_NRVC, 0x0,  IGNORE, IGNORE, IGNORE, 0x7, IGNORE, IGNORE, inst__decode_cstype },
    { "c.sub",      IK_sub,    IGNORE_NRVC, 0x1,  0x0,    0x3,    0x0,    0x4, IGNORE, IGNORE, inst__decode_catype },
    { "c.xor",      IK_xor,    IGNORE_NRVC, 0x1,  0x0,    0x3,    0x1,    0x4, IGNORE, IGNORE, inst__decode_catype },
    { "c.or",       IK_or,     IGNORE_NRVC, 0x1,  0x0,    0x3,    0x2,    0x4, IGNORE, IGNORE, inst__decode_catype },
    { "c.and",      IK_and,    IGNORE_NRVC, 0x1,  0x0,    0x3,    0x3,    0x4, IGNORE, IGNORE, inst__decode_catype },
    { "c.subw",     IK_subw,   IGNORE_NRVC, 0x1,  0x1,    0x3,    0x0,    0x4, IGNORE, IGNORE, inst__decode_catype },
    { "c.addw",     IK_addw,   IGNORE_NRVC, 0x1,  0x1,    0x3,    0x1,    0x4, IGNORE, IGNORE, inst__decode_catype },
    { "c.addi16sp", IK_addi,   IGNORE_NRVC, 0x1,  IGNORE, IGNORE, IGNORE, 0x3, 0x2,    IGNORE, inst__decode_citype3 },
    { "c.addi",     IK_addi,   IGNORE_NRVC, 0x1,  IGNORE, IGNORE, IGNORE, 0x0, IGNORE, IGNORE, inst__decode_citype },
    { "c.addiw",    IK_addiw,  IGNORE_NRVC, 0x1,  IGNORE, IGNORE, IGNORE, 0x1, IGNORE, IGNORE, inst__decode_citype },
    { "c.li",       IK_addi,   IGNORE_NRVC, 0x1,  IGNORE, IGNORE, IGNORE, 0x2, IGNORE, IGNORE, inst__decode_citype },
    { "c.lui",      IK_lui,    IGNORE_NRVC, 0x1,  IGNORE, IGNORE, IGNORE, 0x3, IGNORE, IGNORE, inst__decode_citype5 },
    { "c.srli",     IK_srli,   IGNORE_NRVC, 0x1,  IGNORE, 0x0,    IGNORE, 0x4, IGNORE, IGNORE, inst__decode_cbtype2 },
    { "c.srai",     IK_srai,   IGNORE_NRVC, 0x1,  IGNORE, 0x1,    IGNORE, 0x4, IGNORE, IGNORE, inst__decode_cbtype2 },
    { "c.andi",     IK_andi,   IGNORE_NRVC, 0x1,  IGNORE, 0x2,    IGNORE, 0x4, IGNORE, IGNORE, inst__decode_cbtype2 },
    { "c.j",        IK_jal,    IGNORE_NRVC, 0x1,  IGNORE, IGNORE, IGNORE, 0x5, IGNORE, IGNORE, inst__decode_cjtype },
    { "c.beqz",     IK_beq,    IGNORE_NRVC, 0x1,  IGNORE, IGNORE, IGNORE, 0x6, IGNORE, IGNORE, inst__decode_cbtype },
    { "c.bnez",     IK_bne,    IGNORE_NRVC, 0x1,  IGNORE, IGNORE, IGNORE, 0x7, IGNORE, IGNORE, inst__decode_cbtype },
    { "c.ebreak",   IK_ebreak, IGNORE_NRVC, 0x2,  0x1,    IGNORE, IGNORE, 0x4, 0x00,   0x00,   inst__decode_crtype },
    { "c.jr",       IK_jalr,   IGNORE_NRVC, 0x2,  0x0,    IGNORE, IGNORE, 0x4, IGNORE, 0x00,   inst__decode_crtype },
    { "c.jalr",     IK_jalr,   IGNORE_NRVC, 0x2,  0x1,    IGNORE, IGNORE, 0x4, IGNORE, 0x00,   inst__decode_crtype },
    { "c.mv",       IK_add,    IGNORE_NRVC, 0x2,  0x0,    IGNORE, IGNORE, 0x4, IGNORE, IGNORE, inst__decode_crtype },
    { "c.add",      IK_add,    IGNORE_NRVC, 0x2,  0x1,    IGNORE, IGNORE, 0x4, IGNORE, IGNORE, inst__decode_crtype },
    { "c.slli",     IK_slli,   IGNORE_NRVC, 0x2,  IGNORE, IGNORE, IGNORE, 0x0, IGNORE, IGNORE, inst__decode_citype },
    { "c.fldsp",    IK_fld,    IGNORE_NRVC, 0x2,  IGNORE, IGNORE, IGNORE, 0x1, IGNORE, IGNORE, inst__decode_citype2 },
    { "c.lwsp",     IK_lw,     IGNORE_NRVC, 0x2,  IGNORE, IGNORE, IGNORE, 0x2, IGNORE, IGNORE, inst__decode_citype4 },
    { "c.ldsp",     IK_ld,     IGNORE_NRVC, 0x2,  IGNORE, IGNORE, IGNORE, 0x3, IGNORE, IGNORE, inst__decode_citype2 },
    { "c.fsdsp",    IK_fsd,    IGNORE_NRVC, 0x2,  IGNORE, IGNORE, IGNORE, 0x5, IGNORE, IGNORE, inst__decode_csstype },
    { "c.swsp",     IK_sw,     IGNORE_NRVC, 0x2,  IGNORE, IGNORE, IGNORE, 0x6, IGNORE, IGNORE, inst__decode_csstype2 },
    { "c.sdsp",     IK_sd,     IGNORE_NRVC, 0x2,  IGNORE, IGNORE, IGNORE, 0x7, IGNORE, IGNORE, inst__decode_csstype },
};

void inst_decode(Inst *instp, u32 data)
{
    InstDef def = inst_lookup(data);
    *instp = def.decode(&def, data);
}

InstDef inst_lookup(u32 data)
{
    u16 quadrant = QUADRANT(data);
    if (quadrant == 0x3) {
        u16 opcode     = OPCODE(data);
        u16 funct2     = FUNCT2(data);
        u16 funct3     = FUNCT3(data);
        u16 funct5high = FUNCT5HIGH(data);
        u16 funct5low  = FUNCT5LOW(data);
        u16 funct7     = FUNCT7(data);
        u16 funct12    = FUNCT12(data);

        for (u32 i = 0; i < RVC_START; i++) {
            InstDef def = inst_table[i];
            if (def.opcode     == opcode     &&
               (def.funct2     == funct2     || def.funct2     == IGNORE) &&
               (def.funct3     == funct3     || def.funct3     == IGNORE) &&
               (def.funct5high == funct5high || def.funct5high == IGNORE) &&
               (def.funct5low  == funct5low  || def.funct5low  == IGNORE) &&
               (def.funct7     == funct7     || def.funct7     == IGNORE) &&
               (def.funct12    == funct12    || def.funct12    == IGNORE)) {
                return def;
            }
        }

        fatalf("failed to decode: %d(opcode), %d(funct2), %d(funct3), %d(funct5high), %d(funct5low), %d(funct7), %d(funct12)",
                opcode, funct2, funct3, funct5high, funct5low, funct7, funct12);
    } else {
        u16 copcode     = COPCODE(data);
        u16 cfunct1     = CFUNCT1(data);
        u16 cfunct2high = CFUNCT2HIGH(data);
        u16 cfunct2low  = CFUNCT2LOW(data);
        u16 cfunct3     = CFUNCT3(data);
        u16 cfunct5high = CFUNCT5HIGH(data);
        u16 cfunct5low  = CFUNCT5LOW(data);

        for (u32 i = RVC_START; i < sizeof(inst_table)/sizeof(inst_table[0]); i++) {
            InstDef def = inst_table[i];
            if (def.copcode     == copcode     &&
               (def.cfunct1     == cfunct1     || def.cfunct1     == IGNORE) &&
               (def.cfunct2high == cfunct2high || def.cfunct2high == IGNORE) &&
               (def.cfunct2low  == cfunct2low  || def.cfunct2low  == IGNORE) &&
               (def.cfunct3     == cfunct3     || def.cfunct3     == IGNORE) &&
               (def.cfunct5high == cfunct5high || def.cfunct5high == IGNORE) &&
               (def.cfunct5low  == cfunct5low  || def.cfunct5low  == IGNORE)) {
                return def;
            }
        }

        fatalf("failed to decode: %d(copcode), %d(cfunct2high), %d(cfunct2low), %d(cfunct3), %d(cfunct5high), %d(cfunct5low)",
                copcode, cfunct2high, cfunct2low, cfunct3, cfunct5high, cfunct5low);
    }
}
