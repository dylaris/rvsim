#ifndef CODEGEN_H
#define CODEGEN_H

#include "decode.h"
#include "cpu.h"

typedef void (*EmitFunc)(u8 **, CPUState *, Instr *);
extern EmitFunc emit_table[];

static __ForceInline EmitFunc get_emit_func(InstrKind kind)
{
    return emit_table[kind];
}

#endif // CODEGEN_H
