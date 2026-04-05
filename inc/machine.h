#ifndef MACHINE_H
#define MACHINE_H

#include "cpu.h"
#include "memory.h"
#include "stack.h"
#include "err.h"

typedef struct Machine Machine;
typedef void (*BlockExec)(Machine *);

struct Machine {
    CPUState state;
    Memory mem;

    BlockExec engine;

    bool single_step;
    bool halt;
    Stack(GuestVAddr) breakpoints;
    bool skip_breakpoint;
};

Machine machine_create(void);
void machine_destroy(Machine *machine);
void machine_resolve(Machine *machine);
void machine_step(Machine *machine);
ResultVoid machine_load_bin(Machine *machine, const char *prog, GuestVAddr base);
ResultVoid machine_load_elf(Machine *machine, const char *prog);
ResultVoid machine_init_stack_bin(Machine *machine, u64 stack_size);
ResultVoid machine_init_stack_elf(Machine *machine, u64 stack_size, int argc, char **argv);
ResultVoid machine_trap(Machine *machine);
void machine_add_breakpoint(Machine *machine, GuestVAddr breakpoint);
void machine_del_breakpoint(Machine *machine, u64 index);
bool machine_check_breakpoint(const Machine *machine, GuestVAddr addr);
void machine_repl(Machine *machine);

#endif // MACHINE_H
