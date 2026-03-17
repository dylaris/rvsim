#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "cpu.h"

void exec_block_interp(CPU *cpup);  // Block: linear execution until control flow change

#endif // INTERPRETER_H
