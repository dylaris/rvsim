#include "machine.h"

int main(int argc, char **argv)
{
    if (argc < 2)
        fatalf("Usage: %s <program>", argv[0]);

    Machine machine = machine_create();

#ifdef TEST
    machine_load_bin(&machine, argv[1], 0x80002000);
    machine_init_stack_bin(&machine, KB(64));
#else
    machine_load_elf(&machine, argv[1]);
    machine_init_stack_elf(&machine, MB(32), argc-1, argv+1);
#endif

#if DEBUG
    printf("entry address:   0x%016lx\n", machine.mem.entry);
    printf("machine address: 0x%016lx\n", (HostVAddr) &machine);

    // Run debug repl first
    machine_add_breakpoint(&machine, machine.mem.entry);
    printf("\n===== RISC-V Simulator REPL =====\n");
#endif

    while (true) {
        machine_step(&machine);
        if (IS_TRAP(cpu_get_flow_ctl(&machine.state)))
            machine_trap(&machine);
        cpu_commit_pc(&machine.state);
    }

    machine_destroy(&machine);
    return 0;
}
