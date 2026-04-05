#include "machine.h"
#include "syscall.h"

ResultVoid machine_load_bin(Machine *machine, const char *prog, GuestVAddr base)
{
    ResultVoid res = OK_VOID;
    FILE *f = NULL;

    f = fopen(prog, "rb");
    if (!f)
        return_defer(SYSERR_VOID("fopen"));

    res = mem_load_bin(&machine->mem, f, base);
    if (!res.ok)
        goto defer;

    cpu_set_pc(&machine->state, machine->mem.entry);

defer:
    if (!f)
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
        return_defer(SYSERR_VOID("fopen"));

    res = mem_load_elf(&machine->mem, f);
    if (!res.ok)
        goto defer;

    cpu_set_pc(&machine->state, machine->mem.entry);

defer:
    if (!f)
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
        return_defer(ERR_VOID(vres.err));

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
            return_defer(ERR_VOID(vres.err));
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
        return_defer(ERR_VOID(vres.err));

    GuestVAddr stack_base = vres.value;
    GuestVAddr stack_end  = stack_base + stack_size;
    cpu_set_gpr(&machine->state, GPR_SP, stack_end);

defer:
    return res;
}

static void do_syscall(Machine *machine)
{
    SyscallNr n = (SyscallNr) cpu_get_gpr(&machine->state, GPR_A7);
    SyscallFunc f = syscall_get(n);
    u64 ret = f(machine);
    cpu_set_gpr(&machine->state, GPR_A0, ret);
}

ResultVoid machine_trap(Machine *machine)
{
    ResultVoid res = OK_VOID;
    u64 pc = cpu_get_pc(&machine->state);

    switch (machine->state.flow.ctl) {
    case FLOW_ECALL:
        do_syscall(machine);
        cpu_commit_pc(&machine->state);
        break;

    case FLOW_ILLEGAL_INSTR:
        return_defer(ERR_VOID(SIM_ERR_NEWF("machine_trap", "illegal instruction '0x%08x' @ 0x%016lx", mem_read_u32(pc), pc)));

    case FLOW_LOAD_FAULT:
    case FLOW_STORE_FAULT:
        return_defer(ERR_VOID(SIM_ERR_NEWF("machine_trap", "memory access fault @ 0x%016lx", pc)));

    case FLOW_LOAD_MISALIGN:
    case FLOW_STORE_MISALIGN:
        return_defer(ERR_VOID(SIM_ERR_NEWF("machine_trap", "memory align fault @ 0x%016lx", pc)));

    case FLOW_CRASH:
        return_defer(ERR_VOID(SIM_ERR_NEWF("machine_trap", "this simulator crashed: %s", strerror(errno))));

    default:
        unreachable();
    }

defer:
    return res;
}

BlockExec machine_resolve(Machine *machine)
{
    (void) machine;
    return interp_block;
}

void machine_step(Machine *machine, BlockExec func)
{
    cpu_reset_flow(&machine->state);
    func(&machine->state);
}

void machine_fini(Machine *machine)
{
    mem_clear(&machine->mem);
}
