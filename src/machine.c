#include "machine.h"
#include "interpreter.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

BreakCode machine_step(Machine *mp)
{
    while (1) {
        exec_block_interp(&mp->cpu);

        if (mp->cpu.brkcode == JUMP)
            continue;
        else
            break;
    }
    assert(mp->cpu.brkcode == ECALL);
    return ECALL;
}

void machine_load_program(Machine *mp, const char *prog)
{
    FILE *fp = fopen(prog, "rb");
    if (!fp)
        fatal(strerror(errno));

    mmu_load_elf(&mp->mmu, fp);

    fclose(fp);

    mp->cpu.pc = (u64) mp->mmu.entry;
}
