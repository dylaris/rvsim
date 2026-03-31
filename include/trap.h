#ifndef TRAP_H
#define TRAP_H

#include <setjmp.h>

void trap_init(void);
void trap_enter(jmp_buf *buf);
void trap_leave(void);

#define TRAP_OK    0
#define TRAP_CRASH 1

#endif // TRAP_H
