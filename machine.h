#ifndef MACHINE_H
#define MACHINE_H

#include "cpu.h"
#include "mmu.h"

typedef struct {
    CPU cpu;
    MMU mmu;
} Machine;

void machine_load_program(Machine *mp, const char *prog);

#endif // MACHINE_H
