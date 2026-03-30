#include "machine.h"
#include "interpreter.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

BreakCode machine_step(Machine *mp)
{
    while (1) {
        mp->cpu.brkcode = NONE;
        exec_block_interp(&mp->cpu);
        assert(mp->cpu.brkcode != NONE);

        if (mp->cpu.brkcode == JUMP) {
            mp->cpu.pc = mp->cpu.reenter_pc;
            continue;
        } else
            break;
    }
    mp->cpu.pc = mp->cpu.reenter_pc;
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

void machine_setup(Machine *mp, int argc, char **argv)
{
    u64 stack_size = 32 * 1024 * 1024;
    u64 stack = mmu_alloc(&mp->mmu, stack_size);
    mp->cpu.gp_regs[RI_SP] = stack + stack_size;

    mp->cpu.gp_regs[RI_SP] -= 8; // auxv
    mp->cpu.gp_regs[RI_SP] -= 8; // envp
    mp->cpu.gp_regs[RI_SP] -= 8; // argv end

    u64 args = argc - 1;
    for (int i = args; i > 0; i--) {
        size_t len = strlen(argv[i]);
        u64 addr = mmu_alloc(&mp->mmu, len+1);
        mmu_write(addr, (u8 *)argv[i], len);
        mp->cpu.gp_regs[RI_SP] -= 8; // argv[i]
        mmu_write(mp->cpu.gp_regs[RI_SP], (u8 *)&addr, sizeof(u64));
    }

    mp->cpu.gp_regs[RI_SP] -= 8; // argc
    mmu_write(mp->cpu.gp_regs[RI_SP], (u8 *)&args, sizeof(u64));
}

