#include "machine.h"
#include "syscall.h"
#include "trap.h"

static void machine__load(Machine *machine, const char *prog)
{
    FILE *f = fopen(prog, "rb");
    if (!f)
        fatal(strerror(errno));

    mem_load_elf(&machine->mem, f);

    fclose(f);

    machine->state.pc = (u64) machine->mem.entry;
}

static void machine__setup(Machine *machine, int argc, char **argv)
{
    u64 stack_size = 32 * 1024 * 1024;
    GuestVAddr stack_base = mem_alloc(&machine->mem, stack_size);
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
        GuestVAddr addr = mem_alloc(&machine->mem, len + 1);
        mem_write(addr, (void *) argv[i], len);

        stack_end -= 8; // argv[i]
        cpu_set_gpr(&machine->state, GPR_SP, stack_end);
        mem_write(stack_end, (void *) &addr, sizeof(addr));
    }

    stack_end -= 8; // argc
    cpu_set_gpr(&machine->state, GPR_SP, stack_end);
    mem_write(stack_end, (void *) &argc, sizeof(argc));
}

bool machine_step(Machine *machine)
{
    while (1) {
        cpu_clean_trace(&machine->state);

        if (!interp_block(&machine->state))
            return false;
        assert(machine->state.trace.exit_reason != BLOCK_NONE);

        machine->state.pc = machine->state.trace.target_pc;

        // TODO: more exit reason
        if (machine->state.trace.exit_reason != BLOCK_JUMP)
            break;
    }
    assert(machine->state.trace.exit_reason == BLOCK_ECALL);

    return true;
}

static u64 machine__syscall(Machine *machine)
{
    SyscallNr syscall_nr = (SyscallNr) cpu_get_gpr(&machine->state, GPR_A7);
    u64 ret = do_syscall(machine, syscall_nr);
    cpu_set_gpr(&machine->state, GPR_A0, ret);
    return ret;
}

bool machine_run(Machine *machine)
{
    while (1) {
        if (!machine_step(machine))
            return false;

        // TODO: more trap (only syscall for now)
        machine__syscall(machine);
    }
}

void machine_init(Machine *machine, const char *prog, int argc, char **argv)
{
    trap_init();

    machine__load(machine, prog);
    machine__setup(machine, argc-1, argv+1);
}

void machine_fini(Machine *machine)
{
    (void) machine;
}
