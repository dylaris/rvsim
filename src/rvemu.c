#include "machine.h"
#include "utils.h"

#include <assert.h>

void test_decoder(const char *filename);

int main(int argc, char **argv)
{
    if (argc != 2)
        fatalf("Usage: %s <program>", argv[0]);

    test_decoder(argv[1]);

#if 0
    Machine machine = {0};
    machine_load_program(&machine, argv[1]);

    printf("entry address:   0x%016lx\n", TO_HOST(machine.mmu.entry));
    printf("machine address: 0x%016lx\n", (HostVAddr) &machine);

    while (1) {
        BreakCode brkcode = machine_step(&machine);
        assert(brkcode == ECALL);
    }
#endif

    return 0;
}

void test_decoder(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    assert(fp != NULL);

    u32 data;
    Instruction inst;
    while (fread((void *) &data, 4, 1, fp) == 1) {
        inst_decode(&inst, data);
        inst_print(inst);
    }

    fclose(fp);
}
