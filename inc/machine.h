#ifndef MACHINE_H
#define MACHINE_H

#include "cpu.h"
#include "memory.h"
#include "err.h"
#include "cache.h"
#include "thread_pool.h"
#include "nob.h"

// Store address
typedef struct {
    u64 *items;
    size_t count;
    size_t capacity;
} Stack;

typedef struct Machine Machine;

#ifdef DEBUG
typedef void (*BlockExec)(Machine *);
#else
typedef void (*BlockExec)(CPUState *);
#endif

struct Machine {
    CPUState state;
    Memory mem;
    BlockExec engine;
    ThreadPool *pool;
    Cache *cache;

    bool single_step;
    bool halt;
    Stack breakpoints;
    bool skip_breakpoint;
};

Machine machine_create(void);
void machine_destroy(Machine *machine);
void machine_print(const Machine *machine);
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
