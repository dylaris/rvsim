#include "machine.h"
#include "utils.h"

int main(int argc, char **argv)
{
    if (argc != 2)
        fatalf("Usage: %s <program>", argv[0]);

    Machine machine = {0};
    machine_load_program(&machine, argv[1]);

    printf("entry: 0x%lx\n", machine.mmu.entry);
    printf("machine address: %p\n", &machine);

    return 0;
}
