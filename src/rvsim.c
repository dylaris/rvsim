#include "machine.h"

int main(int argc, char **argv)
{
    if (argc < 2)
        fatalf("Usage: %s <program>", argv[0]);

    ResultVoid res = OK_VOID;
    Machine machine = machine_create();

#ifdef TEST_TVM
    res = machine_load_bin(&machine, argv[1], 0x80000000);
    if (!res.ok)
        goto defer;
    res = machine_init_stack_bin(&machine, KB(64));
    if (!res.ok)
        goto defer;
#else
    res = machine_load_elf(&machine, argv[1]);
    if (!res.ok)
        goto defer;
    res = machine_init_stack_elf(&machine, MB(32), argc-1, argv+1);
    if (!res.ok)
        goto defer;
#endif

#if 1
    printf("entry address:   0x%016lx\n", machine.mem.entry);
    printf("machine address: 0x%016lx\n", (HostVAddr) &machine);
#endif

#if 1
    // run debug repl first
    machine_add_breakpoint(&machine, machine.mem.entry);
    printf("\n===== RISC-V Simulator REPL =====\n");
#endif

    while (true) {
        machine_resolve(&machine);
        machine_step(&machine);
        if (IS_TRAP(cpu_get_flow_ctl(&machine.state)))
            machine_trap(&machine);
        else
            cpu_commit_pc(&machine.state);
    }

defer:
    if (!res.ok)
        sim_err_print(res.err);
    machine_destroy(&machine);
    return 0;
}
