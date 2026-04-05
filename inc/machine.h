#ifndef MACHINE_H
#define MACHINE_H

#include "cpu.h"
#include "memory.h"
#include "err.h"

typedef struct {
    CPUState state;
    Memory mem;
} Machine;

void machine_init(Machine *machine, const char *prog, int argc, char **argv);
void machine_fini(Machine *machine);
BlockExec machine_resolve(Machine *machine);
void machine_step(Machine *machine, BlockExec func);
ResultVoid machine_load_bin(Machine *machine, const char *prog, GuestVAddr base);
ResultVoid machine_load_elf(Machine *machine, const char *prog);
ResultVoid machine_init_stack_bin(Machine *machine, u64 stack_size);
ResultVoid machine_init_stack_elf(Machine *machine, u64 stack_size, int argc, char **argv);
ResultVoid machine_trap(Machine *machine);

#endif // MACHINE_H
