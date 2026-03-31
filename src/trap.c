#include "trap.h"

#include <signal.h>

static jmp_buf *trap_jmp_buf = NULL;

static void trap__signal_handler(int sig)
{
    (void) sig;
    if (trap_jmp_buf)
        longjmp(*trap_jmp_buf, TRAP_CRASH);
}

void trap_init(void)
{
    signal(SIGSEGV, trap__signal_handler);
    signal(SIGILL,  trap__signal_handler);
    signal(SIGBUS,  trap__signal_handler);
    signal(SIGFPE,  trap__signal_handler);
}

void trap_enter(jmp_buf *buf)
{
    trap_jmp_buf = buf;
}

void trap_leave(void)
{
    trap_jmp_buf = NULL;
}
