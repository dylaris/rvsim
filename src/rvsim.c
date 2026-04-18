#include "machine.h"

int main(int argc, char **argv)
{
    if (argc < 3)
        fatalf("Usage: %s [--elf | --bin] <program>", argv[0]);

    Machine machine = machine_create();

    const char *format = argv[1];
    const char *input = argv[2];
    if (strcmp(format, "--elf") == 0) {
        machine_load_elf(&machine, input);
        machine_init_stack_elf(&machine, MB(32), argc-2, argv+2);
    } else if (strcmp(format, "--bin") == 0) {
        machine_load_bin(&machine, input, 0x80002000);
        machine_init_stack_bin(&machine, KB(64));
    } else {
        fatalf("Usage: %s [--elf | --bin] <program>", argv[0]);
    }

#ifdef DEBUG

    printf("entry address:   0x%016lx\n", machine.mem.entry);
    printf("machine address: 0x%016lx\n", (HostVAddr) &machine);

    // Run debug repl first
    machine_add_breakpoint(&machine, machine.mem.entry);
    printf("\n===== RISC-V Simulator REPL =====\n");

    machine_repl(&machine);

#else

    while (true) {
        machine_step(&machine);
        if (IS_TRAP(cpu_get_flow_ctl(&machine.state)))
            machine_trap(&machine);
        cpu_commit_pc(&machine.state);
    }

#endif

    return 0;
}
