#ifndef CPU_H
#define CPU_H

#include "utils.h"

typedef struct {
    u64 gp_regs[32]; // General purpose registers
    u64 pc;
} CPU;

#endif // CPU_H
