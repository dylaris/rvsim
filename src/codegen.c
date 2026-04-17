#include "instr_template.h"
#include "codegen.h"
#include "libtcc.h"

#define FLOW_SET_EXPR(field, expr) \
    sb_appendf(sb, "    cpu_set_flow_%s(state, %s);\n", #field, #expr)
#define FLOW_SET2_EXPR(field, expr) \
    sb_appendf(sb, "        cpu_set_flow_%s(state, %s);\n", #field, #expr)
#define FLOW_SET_VAL(field, val) \
    sb_appendf(sb, "    cpu_set_flow_%s(state, %luULL);\n", #field, (u64) val)
#define FLOW_SET2_VAL(field, val) \
    sb_appendf(sb, "        cpu_set_flow_%s(state, %lu);\n", #field, (u64) val)

#define XREG_SET_VAL(reg, val) \
    sb_appendf(sb, "    cpu_set_gpr(state, %d, %luULL);\n", (reg), (u64) val)
#define XREG_SET_EXPR(reg, expr) \
    sb_appendf(sb, "    cpu_set_gpr(state, %d, %s);\n", (reg), #expr)
#define XREG_GET(reg, name) \
    sb_appendf(sb, "    u64 %s = cpu_get_gpr(state, %d);\n", #name, (reg))

#define FREG_SET_EXPR(reg, expr, view) \
    sb_appendf(sb, "    cpu_set_fpr_%s(state, %d, %s);\n", #view, (reg), #expr)
#define FREG_GET(reg, name, type, view) \
    sb_appendf(sb, "    %s %s = cpu_get_fpr_%s(state, %d);\n", #type, #name, #view, (reg))

#define MEM_READ(addr, type, name) \
    sb_appendf(sb, "    %s %s = mem_read_%s(%s);\n", #type, #name, #type, #addr)
#define MEM_WRITE(addr, type, data) \
    sb_appendf(sb, "    mem_write_%s(%s, (%s) %s);\n", #type, #addr, #type, #data)

#define SIGNATURE(name) static void codegen_func_##name(String_Builder *sb, Instr *instr, Stack *stack, u64 pc)

#define GEN(name, tag, a1, a2, a3, a4) GEN_##tag(name, a1, a2, a3, a4)

#define GEN_EMPTY(name, a1, a2, a3, a4) \
SIGNATURE(name) \
{ \
    (void) sb; \
    (void) instr; \
    (void) stack; \
    (void) pc; \
}

#define GEN_LOAD(name, type, a2, a3, a4) \
SIGNATURE(name) \
{ \
    XREG_GET(instr->rs1, rs1); \
    sb_appendf(sb, "    GuestVAddr addr = rs1 + (i64) %ld;\n", (i64) instr->imm); \
    MEM_READ(addr, type, rd); \
    XREG_SET_EXPR(instr->rd, rd); \
}

#define GEN_OP_IMM(name, a1, sexpr, a3, a4) \
SIGNATURE(name) \
{ \
    XREG_GET(instr->rs1, rs1); \
    sb_appendf(sb, "    u64 val = " #sexpr ";\n", (i64) instr->imm); \
    XREG_SET_EXPR(instr->rd, val); \
}

#define GEN_AUIPC(name, a1, a2, a3, a4) \
SIGNATURE(name) \
{ \
    XREG_SET_VAL(instr->rd, pc + (i64) instr->imm); \
}

#define GEN_STORE(name, type, a2, a3, a4) \
SIGNATURE(name) \
{ \
    XREG_GET(instr->rs1, rs1); \
    XREG_GET(instr->rs2, rs2); \
    sb_appendf(sb, "    GuestVAddr addr = rs1 + (i64) %ld;\n", (i64) instr->imm); \
    MEM_WRITE(addr, type, rs2); \
}

#define GEN_OP_REG(name, expr, a2, a3, a4) \
SIGNATURE(name) \
{ \
    XREG_GET(instr->rs1, rs1); \
    XREG_GET(instr->rs2, rs2); \
    XREG_SET_EXPR(instr->rd, expr); \
}

#define GEN_LUI(name, a1, a2, a3, a4) \
SIGNATURE(name) \
{ \
    XREG_SET_VAL(instr->rd, (i64) instr->imm); \
}

// Block links in branch and jump
#if 1

#define GEN_BRANCH(name, expr, a2, a3, a4) \
SIGNATURE(name) \
{ \
    XREG_GET(instr->rs1, rs1); \
    XREG_GET(instr->rs2, rs2); \
    GuestVAddr target_addr = pc + (i64) instr->imm; \
    sb_appendf(sb, "    if (%s) {\n", #expr); \
    FLOW_SET2_EXPR(ctl, FLOW_BRANCH); \
    FLOW_SET2_VAL(pc, target_addr); \
    if (block_should_link(pc, target_addr)) { \
        da_append(stack, target_addr); \
        sb_appendf(sb, "        goto instr_%lx;\n", target_addr); \
    } else { \
        sb_appendf(sb, "        goto end;\n"); \
    } \
    sb_appendf(sb, "    }\n"); \
}

#define GEN_JUMP(name, a1, saddr, a3, a4) \
SIGNATURE(name) \
{ \
    GuestVAddr return_addr = pc + (instr->rvc ? 2 : 4); \
    XREG_SET_VAL(instr->rd, return_addr); \
    XREG_GET(instr->rs1, rs1); \
    if (instr->kind == instr_jalr) { \
        sb_appendf(sb, "    GuestVAddr target_addr = " #saddr ";\n", (i64) instr->imm); \
        FLOW_SET_EXPR(ctl, FLOW_JUMP); \
        FLOW_SET_EXPR(pc, target_addr); \
        sb_appendf(sb, "    goto end;\n"); \
    } else { \
        GuestVAddr target_addr = pc + (i64) instr->imm; \
        FLOW_SET_EXPR(ctl, FLOW_JUMP); \
        FLOW_SET_VAL(pc, target_addr); \
        if (block_should_link(pc, target_addr)) { \
            da_append(stack, target_addr); \
            sb_appendf(sb, "    goto instr_%lx;\n", target_addr); \
        } else { \
            sb_appendf(sb, "    goto end;\n"); \
        } \
    } \
    sb_appendf(sb, "}\n"); \
}

#else

#define GEN_BRANCH(name, expr, a2, a3, a4) \
SIGNATURE(name) \
{ \
    XREG_GET(instr->rs1, rs1); \
    XREG_GET(instr->rs2, rs2); \
    GuestVAddr target_addr = pc + (i64) instr->imm; \
    sb_appendf(sb, "    if (%s) {\n", #expr); \
    FLOW_SET2_EXPR(ctl, FLOW_BRANCH); \
    FLOW_SET2_VAL(pc, target_addr); \
    sb_appendf(sb, "        goto end;\n"); \
    sb_appendf(sb, "    }\n"); \
}

#define GEN_JUMP(name, a1, saddr, a3, a4) \
SIGNATURE(name) \
{ \
    GuestVAddr return_addr = pc + (instr->rvc ? 2 : 4); \
    XREG_SET_VAL(instr->rd, return_addr); \
    XREG_GET(instr->rs1, rs1); \
    if (instr->kind == instr_jalr) { \
        sb_appendf(sb, "    GuestVAddr target_addr = " #saddr ";\n", (i64) instr->imm); \
        FLOW_SET_EXPR(ctl, FLOW_JUMP); \
        FLOW_SET_EXPR(pc, target_addr); \
    } else { \
        GuestVAddr target_addr = pc + (i64) instr->imm; \
        FLOW_SET_EXPR(ctl, FLOW_JUMP); \
        FLOW_SET_VAL(pc, target_addr); \
    } \
    sb_appendf(sb, "    goto end;\n"); \
    sb_appendf(sb, "}\n"); \
}

#endif

#define GEN_ECALL(name, a1, a2, a3, a4) \
SIGNATURE(name) \
{ \
    sb_appendf(sb, "    GuestVAddr target_addr = %lu;\n", pc + 4); \
    FLOW_SET_EXPR(ctl, FLOW_ECALL); \
    FLOW_SET_EXPR(pc, target_addr); \
    sb_appendf(sb, "    goto end;\n"); \
    sb_appendf(sb, "    }\n"); \
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
    XREG_SET_VAL(instr->rd, 0); \
}

#define GEN_FP_LOAD(name, type, a2, a3, a4) \
SIGNATURE(name) \
{ \
    XREG_GET(instr->rs1, rs1); \
    sb_appendf(sb, "    GuestVAddr addr = rs1 + (i64) %ld;\n", (i64) instr->imm); \
    sb_appendf(sb, "    u64 val = sizeof(" #type ") == 8 ? " \
                   "mem_read_u64(addr) : " \
                   "mem_read_u32(addr) | (-1ULL << 32);\n"); \
    FREG_SET_EXPR(instr->rd, val, q); \
}

#define GEN_FP_STORE(name, type, a2, a3, a4) \
SIGNATURE(name) \
{ \
    FREG_GET(instr->rs2, src, u64, q); \
    XREG_GET(instr->rs1, rs1); \
    sb_appendf(sb, "    GuestVAddr addr = rs1 + (i64) %ld;\n", (i64) instr->imm); \
    MEM_WRITE(addr, type, src); \
}

#define GEN_FP_FMA4(name, view, type, expr, a4) \
SIGNATURE(name) \
{ \
    FREG_GET(instr->rs1, rs1, type, view); \
    FREG_GET(instr->rs2, rs2, type, view); \
    FREG_GET(instr->rs3, rs3, type, view); \
    FREG_SET_EXPR(instr->rd, expr, view); \
}

#define GEN_FP_BINOP(name, view, type, expr, a4) \
SIGNATURE(name) \
{ \
    FREG_GET(instr->rs1, rs1, type, view); \
    FREG_GET(instr->rs2, rs2, type, view); \
    FREG_SET_EXPR(instr->rd, expr, view); \
}

#define GEN_FP_CMP(name, view, type, expr, a4) \
SIGNATURE(name) \
{ \
    FREG_GET(instr->rs1, rs1, type, view); \
    FREG_GET(instr->rs2, rs2, type, view); \
    XREG_SET_EXPR(instr->rd, expr); \
}

// Skip code generation for complicated instruction
#if 1

#define GEN_SKIP(name, a1, a2, a3, a4) \
SIGNATURE(name) \
{ \
    FLOW_SET_EXPR(ctl, FLOW_SKIP_CODEGEN); \
    FLOW_SET_VAL(pc, pc); \
    sb_appendf(sb, "    goto end;\n"); \
    sb_appendf(sb, "}\n"); \
    instr->cfc = true; \
}

#define GEN_OP_HMUL GEN_SKIP
#define GEN_FP_SGNJ GEN_SKIP
#define GEN_FP_XFER_F2I GEN_SKIP
#define GEN_FP_XFER_I2F GEN_SKIP
#define GEN_FP_XFER_F2F GEN_SKIP
#define GEN_FP_CLASS GEN_SKIP

#else

#define GEN_OP_HMUL(name, expr, a2, a3, a4) \
SIGNATURE(name) \
{ \
    XREG_GET(instr->rs1, rs1); \
    XREG_GET(instr->rs2, rs2); \
    XREG_SET_EXPR(instr->rd, expr); \
}

#define GEN_FP_SGNJ(name, view, type, neg, xor) \
SIGNATURE(name) \
{ \
    FREG_GET(instr->rs1, rs1, type, view); \
    FREG_GET(instr->rs2, rs2, type, view); \
    sb_appendf(sb, "bool neg = %s;\n", neg ? "true" : "false"); \
    sb_appendf(sb, "bool xor = %s;\n", xor ? "true" : "false"); \
    FREG_SET_EXPR(instr->rd, (help_fsgnj(rs1, rs2, sizeof(type), neg, xor)), q); \
}

#define GEN_FP_XFER_F2I(name, view, type, expr, a4) \
SIGNATURE(name) \
{ \
    FREG_GET(instr->rs1, src, type, view); \
    FREG_SET_EXPR(instr->rd, expr, view); \
}

#define GEN_FP_XFER_I2F(name, view, expr, a3, a4) \
SIGNATURE(name) \
{ \
    XREG_GET(instr->rs1, src); \
    FREG_SET_EXPR(instr->rd, expr, view); \
}

#define GEN_FP_XFER_F2F(name, sview, dview, type, expr) \
SIGNATURE(name) \
{ \
    FREG_GET(instr->rs1, src, type, view); \
    FREG_SET_EXPR(instr->rd, expr, view); \
}

#define GEN_FP_CLASS(name, view, type, a3, a4) \
SIGNATURE(name) \
{ \
    FREG_GET(instr->rs1, val, type, view); \
    XREG_SET_EXPR(instr->rd, (help_f_classify(val, sizeof(val)))); \
}

#endif

INSTRUCTION_LIST(GEN)

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

#define X(name, tag, a1, a2, a3, a4) [instr_##name] = codegen_func_##name,
typedef void (*CodeGenFunc)(String_Builder *, Instr *, Stack *, u64);
static CodeGenFunc codegen_table[] = { INSTRUCTION_LIST(X) };
#undef X

static TCCState *compile(const char *source)
{
    TCCState *s = tcc_new();
    if (!s) fatal("cannot create tcc state");

    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

    tcc_add_include_path(s, "inc/");

    if (tcc_compile_string(s, source) == -1) {
        tcc_delete(s);
        fatal("tcc compilation failed");
    }

    if (tcc_relocate(s) < 0) {
        tcc_delete(s);
        fatal("tcc relocate failed");
    }

    return s;
}

#if 0

#define CODEGEN_PROLOGUE                                 \
    "#include \"cpu.h\"                              \n" \
    "#include \"mmu.h\"                              \n" \
    "#include \"memory.h\"                           \n" \
    "void start(volatile CPUState *restrict state) { \n" \

#else

#define CODEGEN_PROLOGUE                                \
    "#include <stdint.h>                            \n" \
    "typedef int8_t   i8;                           \n" \
    "typedef int16_t  i16;                          \n" \
    "typedef int32_t  i32;                          \n" \
    "typedef int64_t  i64;                          \n" \
    "typedef uint8_t  u8;                           \n" \
    "typedef uint16_t u16;                          \n" \
    "typedef uint32_t u32;                          \n" \
    "typedef uint64_t u64;                          \n" \
    "typedef float    f32;                          \n" \
    "typedef double   f64;                          \n" \
    "typedef u64 GPR;                               \n" \
    "typedef u64 CSR;                               \n" \
    "typedef union {                                \n" \
    "    u64 q;                                     \n" \
    "    f64 d;                                     \n" \
    "    u32 w;                                     \n" \
    "    f32 s;                                     \n" \
    "} FPR;                                         \n" \
    "#define TRAP_MASK (1 << 4)                     \n" \
    "typedef enum {                                 \n" \
    "    FLOW_NONE           = 0,                   \n" \
    "    FLOW_BRANCH         = 1,                   \n" \
    "    FLOW_JUMP           = 2,                   \n" \
    "    FLOW_SKIP_CODEGEN   = 3,                   \n" \
    "    FLOW_ECALL          = TRAP_MASK | 1,       \n" \
    "    FLOW_ILLEGAL_INSTR  = TRAP_MASK | 2,       \n" \
    "    FLOW_LOAD_MISALIGN  = TRAP_MASK | 3,       \n" \
    "    FLOW_STORE_MISALIGN = TRAP_MASK | 4,       \n" \
    "    FLOW_LOAD_FAULT     = TRAP_MASK | 5,       \n" \
    "    FLOW_STORE_FAULT    = TRAP_MASK | 6,       \n" \
    "    FLOW_CRASH          = TRAP_MASK | 7,       \n" \
    "    FLOW_HALT           = TRAP_MASK | 8,       \n" \
    "    FLOW_CACHE_OVERFLOW = TRAP_MASK | 9,       \n" \
    "} FlowCtrl;                                    \n" \
    "typedef struct {                               \n" \
    "    u64 pc;                                    \n" \
    "    FlowCtrl ctl;                              \n" \
    "} Flow;                                        \n" \
    "typedef struct {                               \n" \
    "    GPR gp_regs[32];                           \n" \
    "    u64 pc;                                    \n" \
    "    Flow flow;                                 \n" \
    "    FPR fp_regs[32];                           \n" \
    "} CPUState;                                    \n" \
    "typedef u64 HostVAddr;                         \n" \
    "typedef u64 GuestVAddr;                        \n" \
    "#define GUEST_MEMORY_OFFSET 0x088800000000ULL  \n" \
    "#define cpu_set_flow_pc(state_, pc_)           ((state_)->flow.pc = (pc_))\n"                                           \
    "#define cpu_set_flow_ctl(state_, ctl_)         ((state_)->flow.ctl = (ctl_))\n"                                         \
    "#define cpu_set_gpr(state_, reg_, val_)        do { if ((reg_) != 0) (state_)->gp_regs[(reg_)] = (val_); } while (0)\n" \
    "#define cpu_get_gpr(state_, reg_)              (((reg_) == 0) ? 0 : (state_)->gp_regs[(reg_)])\n"                       \
    "#define cpu_set_fpr(state_, reg_, val_)        ((state_)->fp_regs[(reg_)] = (val_))\n"                                  \
    "#define cpu_get_fpr(state_, reg_)              ((state_)->fp_regs[(reg_)])\n"                                           \
    "#define cpu_set_fpr_q(state_, reg_, val_)      ((state_)->fp_regs[(reg_)].q = (val_))\n"                                \
    "#define cpu_get_fpr_q(state_, reg_)            ((state_)->fp_regs[(reg_)].q)\n"                                         \
    "#define cpu_set_fpr_w(state_, reg_, val_)      ((state_)->fp_regs[(reg_)].w = (val_))\n"                                \
    "#define cpu_get_fpr_w(state_, reg_)            ((state_)->fp_regs[(reg_)].w)\n"                                         \
    "#define cpu_set_fpr_d(state_, reg_, val_)      ((state_)->fp_regs[(reg_)].d = (val_))\n"                                \
    "#define cpu_get_fpr_d(state_, reg_)            ((state_)->fp_regs[(reg_)].d)\n"                                         \
    "#define cpu_set_fpr_s(state_, reg_, val_)      ((state_)->fp_regs[(reg_)].s = (val_))\n"                                \
    "#define cpu_get_fpr_s(state_, reg_)            ((state_)->fp_regs[(reg_)].s)\n"                                         \
    "#define mmu_to_host(addr)  ((addr) + GUEST_MEMORY_OFFSET)\n"                                                            \
    "#define mmu_to_guest(addr) ((addr) - GUEST_MEMORY_OFFSET)\n"                                                            \
    "#define mem_write_u8(addr, val)                (*(u8*)mmu_to_host(addr) = val)\n"                                       \
    "#define mem_write_u16(addr, val)               (*(u16*)mmu_to_host(addr) = val)\n"                                      \
    "#define mem_write_u32(addr, val)               (*(u32*)mmu_to_host(addr) = val)\n"                                      \
    "#define mem_write_u64(addr, val)               (*(u64*)mmu_to_host(addr) = val)\n"                                      \
    "#define mem_read_u8(addr)                      (*(u8*)mmu_to_host(addr))\n"                                             \
    "#define mem_read_u16(addr)                     (*(u16*)mmu_to_host(addr))\n"                                            \
    "#define mem_read_u32(addr)                     (*(u32*)mmu_to_host(addr))\n"                                            \
    "#define mem_read_u64(addr)                     (*(u64*)mmu_to_host(addr))\n"                                            \
    "#define mem_read_i8(addr)                      (*(i8*)mmu_to_host(addr))\n"                                             \
    "#define mem_read_i16(addr)                     (*(i16*)mmu_to_host(addr))\n"                                            \
    "#define mem_read_i32(addr)                     (*(i32*)mmu_to_host(addr))\n"                                            \
    "#define mem_read_i64(addr)                     (*(i64*)mmu_to_host(addr))\n"                                            \
    "#define help_div(a, b) (((b) == 0) ? (i64)-1  : (((i64)(a) == INT64_MIN && (i64)(b) == (i64)-1) ? INT64_MIN : (i64)(a)  / (i64)(b)))\n" \
    "#define help_rem(a, b) (((b) == 0) ? (i64)(a) : (((i64)(a) == INT64_MIN && (i64)(b) == (i64)-1) ? (i64)0    : (i64)(a) %% (i64)(b)))\n" \
    "void start(volatile CPUState *restrict state) {\n"

#endif

#define CODEGEN_EPILOGUE  \
    "end:             \n" \
    "}                \n" \
    "void end(void) {}\n"

void *gencode(CodeGenerator *cg, TBCache *cache, u64 start_pc)
{
    Stack *links = &cg->links;
    Set *set = &cg->set;
    String_Builder *sb = &cg->sb;

    sb_appendf(sb, CODEGEN_PROLOGUE);

    da_append(links, start_pc);

    while (cg->links.count) {
        u64 pc = da_pop(links);
        bool *exist = ht_find(set, pc);
        if (exist) continue;
        else *ht_put(set, pc) = true;

        sb_appendf(sb, "instr_%lx: {\n", pc);

        Instr instr = {0};
        u32 raw = mem_read_u32(pc);
        assert(decode_instr(raw, &instr));

        codegen_table[instr.kind](sb, &instr, links, pc);

        if (instr.cfc) continue;
        pc += (instr.rvc ? 2 : 4);

        da_append(links, pc);

        sb_appendf(sb, "    goto instr_%lx;\n}\n", pc);
    }

    sb_appendf(sb, CODEGEN_EPILOGUE);

    TCCState *s = compile(sb->items);
    void *start = tcc_get_symbol(s, "start");
    void *end = tcc_get_symbol(s, "end");
    if (!start || !end) {
        tcc_delete(s);
        fatal("symbol 'start' or 'end' not found in source");
    }

    size_t length = (char *) end - (char *) start;
    void *code = tbcache_add(cache, start, length);

    ht_reset(set);
    sb->count = 0;
    links->count = 0;
    tcc_delete(s);

    return code;
}

CodeGenerator *codegen_create(void)
{
    CodeGenerator *cg = malloc(sizeof(CodeGenerator));
    assert(cg && "run out of memory");
    memset(cg, 0, sizeof(CodeGenerator));
    return cg;
}

void codegen_destroy(CodeGenerator *cg)
{
    sb_free(cg->sb);
    da_free(cg->links);
    ht_free(&cg->set);
}
