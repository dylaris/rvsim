#include "template.h"
#include "codegen.h"

#define FLOW_SET_EXPR(expr) \
    sb_appendf(sb, "    cpu_set_flow(state, %s);\n", #expr)
#define FLOW_SET2_EXPR(expr) \
    sb_appendf(sb, "        cpu_set_flow(state, %s);\n", #expr)
#define PC_SET_VAL(val) \
    sb_appendf(sb, "    cpu_set_pc(state, %luULL);\n", (u64) val)
#define PC_SET2_VAL(val) \
    sb_appendf(sb, "        cpu_set_pc(state, %lu);\n", (u64) val)
#define PC_SET_EXPR(expr) \
    sb_appendf(sb, "    cpu_set_pc(state, %s);\n", #expr)
#define PC_SET2_EXPR(expr) \
    sb_appendf(sb, "        cpu_set_pc(state, %s);\n", #expr)

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

#define GEN_EMPTY(name, a1, a2, a3, a4) \
SIGNATURE(name) { }

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
    XREG_SET_VAL(instr->rd, instr->curr_pc + (i64) instr->imm); \
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

#define GEN_BRANCH(name, expr, a2, a3, a4) \
SIGNATURE(name) \
{ \
    XREG_GET(instr->rs1, rs1); \
    XREG_GET(instr->rs2, rs2); \
    GuestVAddr target_addr = instr->curr_pc + (i64) instr->imm; \
    FLOW_SET_EXPR(FLOW_BRANCH_NOT_TAKEN); \
    sb_appendf(sb, "    if (%s) {\n", #expr); \
    FLOW_SET2_EXPR(FLOW_BRANCH_TAKEN); \
    PC_SET2_VAL(target_addr); \
    sb_appendf(sb, "    }\n"); \
}

#define GEN_JUMP(name, a1, saddr, a3, a4) \
SIGNATURE(name) \
{ \
    GuestVAddr return_addr = instr->curr_pc + (instr->rvc ? 2 : 4); \
    XREG_SET_VAL(instr->rd, return_addr); \
    XREG_GET(instr->rs1, rs1); \
    if (instr->kind == instr_jalr) { \
        sb_appendf(sb, "    GuestVAddr target_addr = " #saddr ";\n", (i64) instr->imm); \
        FLOW_SET_EXPR(FLOW_INDIRECT_JUMP); \
        PC_SET_EXPR(target_addr); \
    } else { \
        GuestVAddr target_addr = instr->curr_pc + (i64) instr->imm; \
        FLOW_SET_EXPR(FLOW_DIRECT_JUMP); \
        PC_SET_VAL(target_addr); \
    } \
}

#define GEN_ECALL(name, a1, a2, a3, a4) \
SIGNATURE(name) \
{ \
    sb_appendf(sb, "    GuestVAddr target_addr = %lu;\n", instr->curr_pc + 4); \
    FLOW_SET_EXPR(FLOW_ECALL); \
    PC_SET_EXPR(target_addr); \
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

#define SIGNATURE(name) static void codegen_func_##name(String_Builder *sb, const Instr *instr)
#define GEN(name, tag, a1, a2, a3, a4) GEN_##tag(name, a1, a2, a3, a4)

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
typedef void (*CodeGenFunc)(String_Builder *, const Instr *);
static CodeGenFunc codegen_table[] = { INSTRUCTION_LIST(X) };
#undef X

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
    "#define IS_TRAP(flow) ((flow)>FLOW_TRAP_BEGIN) \n" \
    "typedef enum {                                 \n" \
    "    FLOW_NONE = 0,                             \n" \
    "    FLOW_BRANCH_TAKEN,                         \n" \
    "    FLOW_BRANCH_NOT_TAKEN,                     \n" \
    "    FLOW_DIRECT_JUMP,                          \n" \
    "    FLOW_INDIRECT_JUMP,                        \n" \
    "    FLOW_SKIP_CODEGEN,                         \n" \
    "    FLOW_TRAP_BEGIN,                           \n" \
    "    FLOW_ECALL                                 \n" \
    "} Flow;                                        \n" \
    "typedef struct {                               \n" \
    "    u64 pc;                                    \n" \
    "    Flow flow;                                 \n" \
    "    GPR gp_regs[32];                           \n" \
    "    FPR fp_regs[32];                           \n" \
    "} CPUState;                                    \n" \
    "typedef u64 HostVAddr;                         \n" \
    "typedef u64 GuestVAddr;                        \n" \
    "#define GUEST_MEMORY_OFFSET 0x088800000000ULL  \n" \
    "#define cpu_set_flow(state_, flow_)            ((state_)->flow = (flow_))\n"                                            \
    "#define cpu_set_pc(state_, pc_)                ((state_)->pc = (pc_))\n"                                                \
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

CodeGenerator *codegen_create(void)
{
    CodeGenerator *cg = malloc(sizeof(CodeGenerator));
    assert(cg && "run out of memory");
    memset(cg, 0, sizeof(CodeGenerator));
    cg->code_buffer = mmap(NULL, CODE_BUFFER_CAPACITY,
         PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    cg->free_units = BITMAP_SIZE;
    return cg;
}

void codegen_destroy(CodeGenerator *cg)
{
    sb_free(cg->sb);
    if (cg->s) tcc_delete(cg->s);
    munmap(cg->code_buffer, CODE_BUFFER_CAPACITY);
}

void codegen_compile(CodeGenerator *cg, CacheEntry *entry)
{
    String_Builder *sb = &cg->sb;
    sb->count = 0;
    if (cg->s) {
        tcc_delete(cg->s);
        cg->s = NULL;
    }

    // Generate C code
    sb_appendf(sb, CODEGEN_PROLOGUE);
    da_foreach(Instr, instr, entry) {
        sb_appendf(sb, "instr_%lx: {\n", instr->curr_pc);
        codegen_table[instr->kind](sb, instr);
        if (instr->cfc) {
            sb_appendf(sb, "    goto end;\n}\n");
        } else {
            sb_appendf(sb, "    goto instr_%lx;\n}\n", instr->next_pc);
        }
    }
    sb_appendf(sb, CODEGEN_EPILOGUE);

    // printf("%.*s", (int) sb->count, sb->items);

    // Generate machine code
    cg->s = tcc_new();
    if (!cg->s) fatal("cannot create tcc state");
    tcc_set_output_type(cg->s, TCC_OUTPUT_MEMORY);
    tcc_add_include_path(cg->s, "inc/");
    if (tcc_compile_string(cg->s, sb->items) == -1) {
        tcc_delete(cg->s);
        fatal("tcc compilation failed");
    }
    if (tcc_relocate(cg->s) < 0) {
        tcc_delete(cg->s);
        fatal("tcc relocate failed");
    }

    void *start = tcc_get_symbol(cg->s, "start");
    void *end = tcc_get_symbol(cg->s, "end");
    if (!start || !end) {
        tcc_delete(cg->s);
        fatal("symbol 'start' or 'end' not found in source");
    }
    size_t length = (char *) end - (char *) start;

    u8 *code = codegen_alloc(cg, length);
    if (!code) return;
    memcpy(code, start, length);

    entry->code = code;
    entry->code_length = length;
}

static __ForceInline u64 bytes_to_units(size_t bytes)
{
    if (bytes == 0) return 0;
    return (bytes + CODE_UNIT_SIZE - 1) / CODE_UNIT_SIZE;
}

static __ForceInline u64 ptr_to_unit(CodeGenerator *cg, const u8 *ptr)
{
    return (ptr - cg->code_buffer) / CODE_UNIT_SIZE;
}

u8 *codegen_alloc(CodeGenerator *cg, size_t length)
{
    u64 units_needed = bytes_to_units(length);
    if (units_needed > cg->free_units) return NULL;

    // Use next-fit algorithm
    u64 curr = cg->next;
    u64 free_count = 0;
    u64 alloc_start = 0;
    bool found = false;

    // Found a **continous** free chunk
    for (u64 i = 0; i < BITMAP_SIZE; i++) {
        u64 byte_idx = curr / 8;
        u64 bit_idx  = curr % 8;

        if (!(cg->bitmap[byte_idx] & (1 << bit_idx))) {
            free_count++;
            if (free_count == units_needed) {
                alloc_start = curr - units_needed + 1;
                found = true;
                break;
            }
        } else {
            free_count = 0;
        }

        curr = (curr + 1) % BITMAP_SIZE;
    }

    if (!found) return NULL;

    // Update state
    for (u64 i = 0; i < units_needed; i++) {
        u64 bit = alloc_start + i;
        cg->bitmap[bit / 8] |= (1 << (bit % 8));
    }
    cg->free_units -= units_needed;
    cg->next = (alloc_start + units_needed) % BITMAP_SIZE;

    return cg->code_buffer + alloc_start * CODE_UNIT_SIZE;
}

void codegen_free(CodeGenerator *cg, const u8 *code, size_t length)
{
    u64 start_unit = ptr_to_unit(cg, code);
    if (start_unit >= BITMAP_SIZE) return;

    u64 units_to_free = bytes_to_units(length);
    if (units_to_free == 0) return;

    for (u64 i = 0; i < units_to_free; i++) {
        u64 bit = start_unit + i;
        cg->bitmap[bit / 8] &= ~(1 << (bit % 8));
    }

    cg->free_units += units_to_free;
}
