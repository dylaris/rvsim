#ifndef CODEGEN_H
#define CODEGEN_H

#include "machine.h"

u8 *gen_block(Machine *machine, u64 pc, u64 cache_entry_index);

#endif // CODEGEN_H
