#include "instr_template.h"
#include "machine.h"

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

#define GEN_BRANCH(name, expr, a2, a3, a4) \
SIGNATURE(name) \
{ \
    XREG_GET(instr->rs1, rs1); \
    XREG_GET(instr->rs2, rs2); \
    GuestVAddr target_addr = pc + (i64) instr->imm; \
    sb_appendf(sb, "    if (%s) {\n", #expr); \
    FLOW_SET2_EXPR(ctl, FLOW_BRANCH); \
    FLOW_SET2_VAL(pc, target_addr); \
    sb_appendf(sb, "        goto instr_%lx;\n", target_addr); \
    sb_appendf(sb, "    }\n"); \
    da_append(stack, target_addr); \
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
        sb_appendf(sb, "    goto instr_%lx;\n", target_addr); \
        da_append(stack, target_addr); \
    } \
    sb_appendf(sb, "}\n"); \
}

// #define GEN_BRANCH(name, expr, a2, a3, a4)
// SIGNATURE(name)
// {
//     XREG_GET(instr->rs1, rs1);
//     XREG_GET(instr->rs2, rs2);
//     GuestVAddr target_addr = pc + (i64) instr->imm;
//     sb_appendf(sb, "    if (%s) {\n", #expr);
//     FLOW_SET2_EXPR(ctl, FLOW_BRANCH);
//     FLOW_SET2_VAL(pc, target_addr);
//     sb_appendf(sb, "        goto end;\n");
//     sb_appendf(sb, "    }\n");
// }
//
// #define GEN_JUMP(name, a1, saddr, a3, a4)
// SIGNATURE(name)
// {
//     GuestVAddr return_addr = pc + (instr->rvc ? 2 : 4);
//     XREG_SET_VAL(instr->rd, return_addr);
//     XREG_GET(instr->rs1, rs1);
//     if (instr->kind == instr_jalr)
//         sb_appendf(sb, "    GuestVAddr target_addr = " #saddr ";\n", (i64) instr->imm);
//         FLOW_SET_EXPR(ctl, FLOW_JUMP);
//         FLOW_SET_EXPR(pc, target_addr);
//     } else {
//         GuestVAddr target_addr = pc + (i64) instr->imm;
//         FLOW_SET_EXPR(ctl, FLOW_JUMP);
//         FLOW_SET_VAL(pc, target_addr);
//     }
//     sb_appendf(sb, "    goto end;\n");
//     sb_appendf(sb, "}\n");
// }

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

#define GEN_FP_SGNJ GEN_SKIP
#define GEN_FP_XFER_F2I GEN_SKIP
#define GEN_FP_XFER_I2F GEN_SKIP
#define GEN_FP_XFER_F2F GEN_SKIP
#define GEN_FP_CLASS GEN_SKIP

#else

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

#define BINBUF_CAP (1024 * 1024)
static u8 elfbuf[BINBUF_CAP] = {0};

u8 *compile(Machine *machine, String_View sv);

u8 *gen_code(Machine *machine)
{
    static String_Builder sb = {0};
    static Ht(u64, bool) ht = {0};
    static Stack stack = {0};
    static Instr instr = {0};

    // prologue
    sb_appendf(&sb, "#include \"cpu.h\"\n");
    sb_appendf(&sb, "#include \"mmu.h\"\n");
    sb_appendf(&sb, "#include \"memory.h\"\n");
    sb_appendf(&sb, "#include \"interp.h\"\n");
    sb_appendf(&sb, "void start(volatile CPUState *restrict state)\n{\n");

    da_append(&stack, cpu_get_pc(&machine->state));

    while (stack.count) {
        u64 pc = da_pop(&stack);
        bool *exist = ht_find(&ht, pc);
        if (exist)
            continue;
        else
            *ht_put(&ht, pc) = true;

        sb_appendf(&sb, "instr_%lx: {\n", pc);

        u32 raw = mem_read_u32(pc);
        assert(decode_instr(raw, &instr));
        if (instr.kind == instr_ebreak) {
            printf("[%lx] %s\n", pc, instr_to_string(&instr));
            abort();
        }
        codegen_table[instr.kind](&sb, &instr, &stack, pc);

        if (instr.cfc)
            continue;

        pc += (instr.rvc ? 2 : 4);
        da_append(&stack, pc);

        sb_appendf(&sb, "    goto instr_%lx;\n}\n", pc);
    }

    sb_appendf(&sb, "end:\n    ;\n}\n");

    String_View sv = {
        .data = sb.items,
        .count = sb.count,
    };

    // printf(SV_Fmt, SV_Arg(sv));
    // fflush(stdout);
    // abort();

    u8 *code = compile(machine, sv);

    ht_reset(&ht);
    stack.count = 0;
    sb.count = 0;
    return code;
}

u8 *compile(Machine *m, String_View sv)
{
    const char *source = sv.data;
    size_t len = sv.count;

    int saved_stdout = dup(STDOUT_FILENO);
    int outp[2];

    if (pipe(outp) != 0) fatal("cannot make a pipe");
    dup2(outp[1], STDOUT_FILENO);
    close(outp[1]);

    FILE *f;
    f = popen("clang -O3 -c -Iinc/ -xc -o /dev/stdout -", "w");
    if (f == NULL) fatal("cannot compile program");
    fwrite(source, 1, len, f);
    pclose(f);
    fflush(stdout);

    (void) read(outp[0], elfbuf, BINBUF_CAP);
    close(outp[0]);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);

    ELFHeader *ehdr = (ELFHeader *)elfbuf;

    /**
     * for some instructions, clang will generate a corresponding .rodata section.
     * this means we need to write a mini-linker that puts the .rodata section into
     * memory, takes its actual address, and uses symbols and relocations to apply
     * it back to the corresponding location in the .text section.
     */

    i64 text_idx = 0, symtab_idx = 0, rela_idx = 0, rodata_idx = 0;
    {
        u64 shstr_shoff = ehdr->shoff + ehdr->shstrndx * sizeof(SecHeader);
        SecHeader *shstr_shdr = (SecHeader *)(elfbuf + shstr_shoff);
        assert(ehdr->shnum != 0);

        for (i64 idx = 0; idx < ehdr->shnum; idx++) {
            u64 shoff = ehdr->shoff + idx * sizeof(SecHeader);
            SecHeader *shdr = (SecHeader *)(elfbuf + shoff);
            char *str = (char *)(elfbuf + shstr_shdr->offset + shdr->name);
            if (strcmp(str, ".text") == 0) text_idx = idx;
            if (strcmp(str, ".rela.text") == 0) rela_idx = idx;
            if (strncmp(str, ".rodata.", strlen(".rodata.")) == 0) rodata_idx = idx;
            if (strcmp(str, ".symtab") == 0) symtab_idx = idx;
        }
    }

    assert(text_idx != 0 && symtab_idx != 0);

    u64 text_shoff = ehdr->shoff + text_idx * sizeof(SecHeader);
    SecHeader *text_shdr = (SecHeader *)(elfbuf + text_shoff);

    if (rela_idx == 0 || rodata_idx == 0) {
        return cache_add(&m->cache, m->state.pc, elfbuf + text_shdr->offset,
                         text_shdr->size, text_shdr->addralign);
    }

    u64 text_addr = 0, rodata_addr = 0;
    {
        u64 shoff = ehdr->shoff + rodata_idx * sizeof(SecHeader);
        SecHeader *shdr = (SecHeader *)(elfbuf + shoff);
        rodata_addr = (u64)cache_add(&m->cache, m->state.pc, elfbuf + shdr->offset,
                  shdr->size, shdr->addralign);
        text_addr = (u64)cache_add(&m->cache, m->state.pc, elfbuf + text_shdr->offset,
                                   text_shdr->size, text_shdr->addralign);
    }

    // apply relocations to .text section.
    {
        u64 shoff = ehdr->shoff + rela_idx * sizeof(SecHeader);
        SecHeader *shdr = (SecHeader *)(elfbuf + shoff);
        i64 rels = shdr->size / sizeof(Rela);

        u64 symtab_shoff = ehdr->shoff + symtab_idx * sizeof(SecHeader);
        SecHeader *symtab_shdr = (SecHeader *)(elfbuf + symtab_shoff);

        for (i64 idx = 0; idx < rels; idx++) {
#ifndef __x86_64__
            fatal("only support x86_64 for now");
#endif
            Rela *rel = (Rela *)(elfbuf + shdr->offset + idx * sizeof(Rela));
            assert(rel->type == R_X86_64_PC32);

            Sym *sym = (Sym *)(elfbuf + symtab_shdr->offset + rel->sym * sizeof(Sym));
            u32 *loc = (u32 *)(text_addr + rel->offset);
            u64 S = rodata_addr + sym->value; /* actual virtual address of symbol */
            u64 P = text_addr + rel->offset; /* relocation address */
            i64 A = rel->addend;
            *loc = (u32)(S + A - P);
            u64 rip_addr = P - A;
            assert(rip_addr + *(i32 *)loc == S);
        }
    }

    return (u8 *)text_addr;
}

