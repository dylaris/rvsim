#ifndef MACHINE_H
#define MACHINE_H

#include "cpu.h"
#include "memory.h"
#include "tbcache.h"
#include "dbcache.h"
#include "codegen.h"

typedef struct Machine Machine;
typedef void (*BlockExec)(Machine *);

struct Machine {
    // CPUState must be the first member, so that
    // (Machine *) can convert to (CPUState *) safely
    CPUState state;
    Memory *mem;
    BlockExec engine;
    TBCache *tbcache;
    DBCache *dbcache;
    CodeGenerator *codegen;
    Stack breakpoints;
};

Machine machine_create(void);
void machine_destroy(Machine *machine);
void machine_print(const Machine *machine);
void machine_step(Machine *machine);
void machine_load_bin(Machine *machine, const char *prog, GuestVAddr base);
void machine_load_elf(Machine *machine, const char *prog);
void machine_init_stack_bin(Machine *machine, u64 stack_size);
void machine_init_stack_elf(Machine *machine, u64 stack_size, int argc, char **argv);
void machine_trap(Machine *machine);
void machine_add_breakpoint(Machine *machine, GuestVAddr breakpoint);
void machine_del_breakpoint(Machine *machine, u64 index);
bool machine_check_breakpoint(const Machine *machine, GuestVAddr addr);
void machine_repl(Machine *machine);

#endif // MACHINE_H
