#ifndef STATE_H
#define STATE_H

#include "utils.h"

typedef struct {
    u64 gp_regs[32]; // General purpose registers
    u64 pc;
} State;

#endif // STATE_H
