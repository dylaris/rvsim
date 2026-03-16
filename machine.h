#ifndef MACHINE_H
#define MACHINE_H

#include "state.h"
#include "mmu.h"

typedef struct {
    State state;
    MMU mmu;
} Machine;

void machine_load_program(Machine *mp, const char *prog);

#endif // MACHINE_H
