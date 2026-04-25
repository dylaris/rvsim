#ifndef SYSCALL_H
#define SYSCALL_H

#include "machine.h"

#include "template.h"

#define OLD_SYSCALL_THRESHOLD 1024

#define X(name, number) SYS_##name = number,
typedef enum {
    SYSCALL_LIST(X)
} SyscallNr;
#undef X

typedef u64 (*SyscallFunc)(Machine *);

const char *syscall_to_string(SyscallNr n);
u64 do_syscall(Machine *machine);

#endif // SYSCALL_H
