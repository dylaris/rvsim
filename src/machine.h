#ifndef MACHINE_H
#define MACHINE_H

#include "cpu.h"
#include "mmu.h"

typedef struct {
    CPU cpu;
    MMU mmu;
} Machine;

void machine_load_program(Machine *mp, const char *prog);
BreakCode machine_step(Machine *mp);
void machine_setup(Machine *mp, int argc, char **argv);

#endif // MACHINE_H
