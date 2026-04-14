#include "machine.h"
#include "syscall.h"

ResultVoid machine_load_bin(Machine *machine, const char *prog, GuestVAddr base)
{
    ResultVoid res = OK_VOID;
    FILE *f = NULL;

    f = fopen(prog, "rb");
    if (!f)
        return__defer(SYSERR_VOID("fopen"));

    res = mem_load_bin(&machine->mem, f, base);
    if (!res.ok)
        goto defer;

    cpu_set_pc(&machine->state, machine->mem.entry);
    cpu_set_flow_pc(&machine->state, machine->mem.entry);

defer:
    if (f)
        fclose(f);
    if (!res.ok)
        mem_clear(&machine->mem);
    return res;
}

ResultVoid machine_load_elf(Machine *machine, const char *prog)
{
    ResultVoid res = OK_VOID;
    FILE *f = NULL;

    f = fopen(prog, "rb");
    if (!f)
        return__defer(SYSERR_VOID("fopen"));

    res = mem_load_elf(&machine->mem, f);
    if (!res.ok)
        goto defer;

    cpu_set_pc(&machine->state, machine->mem.entry);
    cpu_set_flow_pc(&machine->state, machine->mem.entry);

defer:
    if (f)
        fclose(f);
    if (!res.ok)
        mem_clear(&machine->mem);
    return res;
}

ResultVoid machine_init_stack_elf(Machine *machine, u64 stack_size, int argc, char **argv)
{
    ResultVoid res = OK_VOID;
    Result(GuestVAddr) vres = OK(GuestVAddr, 0);

    vres = mem_alloc(&machine->mem, stack_size);
    if (!vres.ok)
        return__defer(ERR_VOID(vres.err));

    GuestVAddr stack_base = vres.value;
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
        vres = mem_alloc(&machine->mem, len + 1);
        if (!vres.ok)
            return__defer(ERR_VOID(vres.err));
        GuestVAddr addr = vres.value;
        mem_write(addr, (void *) argv[i], len);

        stack_end -= 8; // argv[i]
        cpu_set_gpr(&machine->state, GPR_SP, stack_end);
        mem_write(stack_end, (void *) &addr, sizeof(addr));
    }

    stack_end -= 8; // argc
    cpu_set_gpr(&machine->state, GPR_SP, stack_end);
    mem_write(stack_end, (void *) &argc, sizeof(argc));

defer:
    return res;
}

ResultVoid machine_init_stack_bin(Machine *machine, u64 stack_size)
{
    ResultVoid res = OK_VOID;
    Result(GuestVAddr) vres = OK(GuestVAddr, 0);

    vres = mem_alloc(&machine->mem, stack_size);
    if (!vres.ok)
        return__defer(ERR_VOID(vres.err));

    GuestVAddr stack_base = vres.value;
    GuestVAddr stack_end  = stack_base + stack_size;
    cpu_set_gpr(&machine->state, GPR_SP, stack_end);

defer:
    return res;
}

ResultVoid machine_trap(Machine *machine)
{
    ResultVoid res = OK_VOID;
    u64 pc = cpu_get_pc(&machine->state);

    switch (machine->state.flow.ctl) {
    case FLOW_ECALL:
        do_syscall(machine);
        break;

    case FLOW_ILLEGAL_INSTR:
        return__defer(ERR_VOID(SIM_ERR_NEWF("machine_trap", "illegal instruction '0x%08x' @ 0x%016lx", mem_read_u32(pc), pc)));

    case FLOW_LOAD_FAULT:
    case FLOW_STORE_FAULT:
        return__defer(ERR_VOID(SIM_ERR_NEWF("machine_trap", "memory access fault @ 0x%016lx", pc)));

    case FLOW_LOAD_MISALIGN:
    case FLOW_STORE_MISALIGN:
        return__defer(ERR_VOID(SIM_ERR_NEWF("machine_trap", "memory align fault @ 0x%016lx", pc)));

    case FLOW_CRASH:
        return__defer(ERR_VOID(SIM_ERR_NEWF("machine_trap", "this simulator crashed: %s", strerror(errno))));

    case FLOW_HALT:
        machine->skip_breakpoint = false;
        machine->single_step = false;
        machine->halt = true;
        machine_repl(machine);
        machine->halt = false;
        break;

    default:
        unreachable();
    }

defer:
    return res;
}

// void machine_resolve(Machine *machine)
// {
// #ifdef DEBUG
//     if (machine->single_step) {
//         machine->engine = interp_single;
//         return;
//     }
//     machine->engine = interp_block;
// #else
//     // machine->engine = interp_block;
//
//     CacheEntry *entry = cache_lookup(&machine->cache, cpu_get_pc(&machine->state));
//     if (!entry->code && entry->hot >= CACHE_HOT_COUNT)
//         entry->code = gen_code(machine);
//     machine->engine = entry->code != NULL ? (BlockExec) entry->code : interp_block;
// #endif
// }

void machine_step(Machine *machine)
{
#ifdef DEBUG

    if (machine->single_step) {
        machine->engine = interp_single;
        return;
    }
    machine->engine = interp_block;
    cpu_reset_flow_ctl(&machine->state);
    machine->engine(machine);

#else
    while (true) {
cache_lookup:
        ;
        CacheEntry *entry = cache_lookup(&machine->cache, cpu_get_pc(&machine->state));
        if (!entry->code && entry->hot >= CACHE_HOT_COUNT)
            entry->code = gen_code(machine);
        machine->engine = entry->code != NULL ? (BlockExec) entry->code : interp_block;

exec:
        cpu_reset_flow_ctl(&machine->state);
        machine->engine(&machine->state);

        switch (cpu_get_flow_ctl(&machine->state)) {
        case FLOW_BRANCH:
        case FLOW_JUMP:
            cpu_commit_pc(&machine->state);
            goto cache_lookup;
        case FLOW_SKIP_CODEGEN:
            machine->engine = interp_block;
            cpu_commit_pc(&machine->state);
            goto exec;
        case FLOW_ECALL:
            return;
        default:
            unreachable();
        }
    }

#endif
}


void machine_print(const Machine *machine)
{
    printf("flow.pc: 0x%lx\n", cpu_get_flow_pc(&machine->state));
    // printf("ctl:     %d\n", cpu_get_flow_ctl(&machine->state));
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
        .mem         = (Memory) {0},
        .engine      = NULL,
        .cache       = cache_create(CACHE_SIZE),
        .halt        = false,
        .single_step = false,
        .breakpoints = {0},
    };
}

void machine_destroy(Machine *machine)
{
    mem_clear(&machine->mem);
    da_free(machine->breakpoints);
    cache_destroy(&machine->cache);
}

void machine_add_breakpoint(Machine *machine, GuestVAddr breakpoint)
{
    da_append(&machine->breakpoints, breakpoint);
}

void machine_del_breakpoint(Machine *machine, u64 index)
{
    u64 len = machine->breakpoints.count;
    if (len == 0 || index >= len)
        return;
    if (index < len - 1)
        nob_swap(u64, machine->breakpoints.items[index], machine->breakpoints.items[len - 1]);
    da_pop(&machine->breakpoints);
}

bool machine_check_breakpoint(const Machine *machine, GuestVAddr addr)
{
    for (u64 i = 0; i < machine->breakpoints.count; i++) {
        GuestVAddr breakpoint = machine->breakpoints.items[i];
        if (addr == breakpoint)
            return true;
    }
    return false;
}

void machine_repl(Machine *machine)
{
    char input[256];
    char cmd;
    GuestVAddr addr;

    if (!machine->halt)
        return;

    while (1) {
        printf(">>> ");
        fflush(stdout);

        // parse input
        if (!fgets(input, sizeof(input), stdin))
            exit(0);
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0)
            continue;
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
            machine->skip_breakpoint = true;
            printf("[cont] pc = 0x%016lx\n", cpu_get_pc(&machine->state));
            return;

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
            machine->single_step = true;
            printf("[step] pc = 0x%016lx\n", cpu_get_pc(&machine->state));
            return;

        case 'q':
            printf("[debug] quit\n");
            exit(0);

        default:
            continue;
        }
    }
}
