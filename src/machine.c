#include "machine.h"
#include "syscall.h"

void machine_load_bin(Machine *machine, const char *prog, GuestVAddr base)
{
    FILE *f = fopen(prog, "rb");
    if (!f) fatalf("fopen: %s", strerror(errno));

    mem_load_bin(machine->mem, f, base);

    cpu_set_pc(&machine->state, machine->mem->entry);
    cpu_set_flow_pc(&machine->state, machine->mem->entry);

    fclose(f);
}

void machine_load_elf(Machine *machine, const char *prog)
{
    FILE *f = fopen(prog, "rb");
    if (!f) fatalf("fopen: %s", strerror(errno));

    mem_load_elf(machine->mem, f);

    cpu_set_pc(&machine->state, machine->mem->entry);
    cpu_set_flow_pc(&machine->state, machine->mem->entry);

    fclose(f);
}

void machine_init_stack_elf(Machine *machine, u64 stack_size, int argc, char **argv)
{
    GuestVAddr stack_base = mem_alloc(machine->mem, stack_size);
    GuestVAddr stack_end  = stack_base + stack_size;

    cpu_set_gpr(&machine->state, GPR_SP, stack_end);

    stack_end -= 8; // auxv
    cpu_set_gpr(&machine->state, GPR_SP, stack_end);

    stack_end -= 8; // envp
    cpu_set_gpr(&machine->state, GPR_SP, stack_end);

    stack_end -= 8; // argv end
    cpu_set_gpr(&machine->state, GPR_SP, stack_end);

    for (int i = argc - 1; i >= 0; i--) {
        size_t len = strlen(argv[i]);
        GuestVAddr addr = mem_alloc(machine->mem, len + 1);
        mem_write(addr, (void *) argv[i], len);

        stack_end -= 8; // argv[i]
        cpu_set_gpr(&machine->state, GPR_SP, stack_end);
        mem_write(stack_end, (void *) &addr, sizeof(addr));
    }

    stack_end -= 8; // argc
    cpu_set_gpr(&machine->state, GPR_SP, stack_end);
    mem_write(stack_end, (void *) &argc, sizeof(argc));
}

void machine_init_stack_bin(Machine *machine, u64 stack_size)
{
    GuestVAddr stack_base = mem_alloc(machine->mem, stack_size);
    GuestVAddr stack_end  = stack_base + stack_size;
    cpu_set_gpr(&machine->state, GPR_SP, stack_end);
}

void machine_trap(Machine *machine)
{
    u64 pc = cpu_get_pc(&machine->state);

    switch (machine->state.flow.ctl) {
    case FLOW_ECALL:
        do_syscall(machine);
        return;

    case FLOW_ILLEGAL_INSTR:
        fatalf("illegal instruction '0x%08x' @ 0x%016lx", mem_read_u32(pc), pc);

    default:
        unreachable();
    }
}

void machine_step(Machine *machine)
{
//     while (true) {
// #ifdef INTERP
//     machine->engine = interp_block;
// #else
//     #if defined(ENABLE_DBCACHE) || defined(ENABLE_TBCACHE)
//         u64 pc = cpu_get_pc(&machine->state);
//         u64 hit_count = profile_block_hit(pc);
//     #endif
//
//     #ifdef ENABLE_DBCACHE
//         DBCacheEntry *dbentry = dbcache_lookup(machine->dbcache, pc);
//         if (!dbentry && !machine->dbcache->full) dbentry = dbcache_add(machine->dbcache, pc);
//         machine->engine = interp_block;
//     #endif // ENABLE_DBCACHE
//
//     #ifdef ENABLE_TBCACHE
//         TBCacheEntry *tbentry = tbcache_lookup(machine->tbcache, pc);
//         if (!tbentry->code && hit_count >= TBCACHE_HOT_COUNT) {
//             tbentry->code = gencode(machine->codegen, machine->tbcache, pc);
//         }
//         machine->engine = tbentry->code != NULL ? (BlockExec) tbentry->code : interp_block;
//     #endif // ENABLE_TBCACHE
// #endif // INTERP
//
// exec:
//         cpu_reset_flow_ctl(&machine->state);
//         machine->engine(machine);
//
//         switch (cpu_get_flow_ctl(&machine->state)) {
//         case FLOW_BRANCH:
//         case FLOW_JUMP:
//             cpu_commit_pc(&machine->state);
//             break;
//         case FLOW_SKIP_CODEGEN:
//             machine->engine = interp_block;
//             cpu_commit_pc(&machine->state);
//             goto exec;
//         case FLOW_ECALL:
//             return;
//         default:
//             printf("%d\n", cpu_get_flow_ctl(&machine->state));
//             unreachable();
//         }
//     }
}

void machine_print(const Machine *machine)
{
    printf("flow.pc: 0x%lx\n", cpu_get_flow_pc(&machine->state));
    printf("ctl:     %d\n", cpu_get_flow_ctl(&machine->state));
    printf("PC:      0x%lx\n", cpu_get_pc(&machine->state));
    printf("ZERO:    0x%lx\n", cpu_get_gpr(&machine->state, GPR_ZERO));
    printf("RA:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_RA));
    printf("SP:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_SP));
    printf("GP:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_GP));
    printf("TP:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_TP));
    printf("T0:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_T0));
    printf("T1:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_T1));
    printf("T2:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_T2));
    printf("S0:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_S0));
    printf("S1:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_S1));
    printf("A0:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_A0));
    printf("A1:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_A1));
    printf("A2:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_A2));
    printf("A3:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_A3));
    printf("A4:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_A4));
    printf("A5:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_A5));
    printf("A6:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_A6));
    printf("A7:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_A7));
    printf("S2:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_S2));
    printf("S3:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_S3));
    printf("S4:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_S4));
    printf("S5:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_S5));
    printf("S6:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_S6));
    printf("S7:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_S7));
    printf("S8:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_S8));
    printf("S9:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_S9));
    printf("S10:     0x%lx\n", cpu_get_gpr(&machine->state, GPR_S10));
    printf("S11:     0x%lx\n", cpu_get_gpr(&machine->state, GPR_S11));
    printf("T3:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_T3));
    printf("T4:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_T4));
    printf("T5:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_T5));
    printf("T6:      0x%lx\n", cpu_get_gpr(&machine->state, GPR_T6));
    printf("==============\n");
    fflush(stdout);
}

Machine machine_create(void)
{
    return (Machine) {
        .state       = (CPUState) {0},
        .mem         = mem_create(),
        .engine      = NULL,
        .tbcache     = tbcache_create(16),
        .dbcache     = dbcache_create(),
        .codegen     = codegen_create(),
        .breakpoints = {0},
    };
}

void machine_destroy(Machine *machine)
{
    profile_free();
    mem_destroy(machine->mem);
    da_free(machine->breakpoints);
    codegen_destroy(machine->codegen);
    tbcache_destroy(machine->tbcache);
    dbcache_destroy(machine->dbcache);
}

void machine_add_breakpoint(Machine *machine, GuestVAddr breakpoint)
{
    da_append(&machine->breakpoints, breakpoint);
}

void machine_del_breakpoint(Machine *machine, u64 index)
{
    u64 len = machine->breakpoints.count;
    if (len == 0 || index >= len) return;
    if (index < len - 1) {
        nob_swap(u64, machine->breakpoints.items[index], machine->breakpoints.items[len - 1]);
    }
    da_pop(&machine->breakpoints);
}

bool machine_check_breakpoint(const Machine *machine, GuestVAddr addr)
{
    for (u64 i = 0; i < machine->breakpoints.count; i++) {
        GuestVAddr breakpoint = machine->breakpoints.items[i];
        if (addr == breakpoint) return true;
    }
    return false;
}

void machine_repl(Machine *machine)
{
    char input[256];
    char cmd;
    GuestVAddr addr;

    while (true) {
        printf(">>> ");
        fflush(stdout);

        // parse input
        if (!fgets(input, sizeof(input), stdin)) exit(0);
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0) continue;
        sscanf(input, "%c", &cmd);

        // handle command
        switch (cmd) {
        case 'h':
            printf("Commands:\n");
            printf("  h          print this message\n");
            printf("  b <addr>   set breakpoint at address\n");
            printf("  c          continue running\n");
            printf("  r          show registers\n");
            printf("  p <addr>   show memory at address\n");
            printf("  n          step next instruction\n");
            printf("  q          quit\n");
            printf("====================================\n");
            break;

        case 'b':
            if (sscanf(input, "b %lx", &addr) == 1) {
                machine_add_breakpoint(machine, addr);
                printf("[debug] breakpoint set at 0x%016lx\n", addr);
            } else
                printf("usage: b 0x1234\n");
            break;

        case 'c':
            printf("[cont] pc = 0x%016lx\n", cpu_get_pc(&machine->state));
            do {
                interp(machine);
            } while (!machine_check_breakpoint(machine, cpu_get_pc(&machine->state)));
            break;

        case 'r':
            printf("PC:   0x%lx\n", cpu_get_pc(&machine->state));
            printf("ZERO: 0x%lx\n", cpu_get_gpr(&machine->state, GPR_ZERO));
            printf("RA:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_RA));
            printf("SP:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_SP));
            printf("GP:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_GP));
            printf("TP:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_TP));
            printf("T0:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_T0));
            printf("T1:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_T1));
            printf("T2:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_T2));
            printf("S0:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_S0));
            printf("S1:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_S1));
            printf("A0:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_A0));
            printf("A1:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_A1));
            printf("A2:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_A2));
            printf("A3:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_A3));
            printf("A4:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_A4));
            printf("A5:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_A5));
            printf("A6:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_A6));
            printf("A7:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_A7));
            printf("S2:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_S2));
            printf("S3:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_S3));
            printf("S4:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_S4));
            printf("S5:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_S5));
            printf("S6:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_S6));
            printf("S7:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_S7));
            printf("S8:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_S8));
            printf("S9:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_S9));
            printf("S10:  0x%lx\n", cpu_get_gpr(&machine->state, GPR_S10));
            printf("S11:  0x%lx\n", cpu_get_gpr(&machine->state, GPR_S11));
            printf("T3:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_T3));
            printf("T4:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_T4));
            printf("T5:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_T5));
            printf("T6:   0x%lx\n", cpu_get_gpr(&machine->state, GPR_T6));
            break;

        case 'p':
            if (sscanf(input, "p %lx", &addr) == 1) {
                u64 val = mem_read_u64(addr);
                printf("[mem] 0x%016lx = 0x%016lx\n", addr, val);
            } else
                printf("usage: p 0x80000000\n");
            break;

        case 'n':
            printf("[step] pc = 0x%016lx\n", cpu_get_pc(&machine->state));
            interp(machine);
            break;

        case 'q':
            printf("[debug] quit\n");
            exit(0);

        default:
            continue;
        }
    }
}
