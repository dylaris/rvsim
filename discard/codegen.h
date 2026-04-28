#ifndef CODEGEN_H
#define CODEGEN_H

#include "common.h"
#include "nob.h"
#include "libtcc.h"

#define CODE_UNIT_SIZE 64
#define CODE_BUFFER_CAPACITY (MB(32))
#define BITMAP_SIZE (CODE_BUFFER_CAPACITY / CODE_UNIT_SIZE / 8)

typedef struct CacheEntry CacheEntry;

typedef struct {
    String_Builder sb;
    TCCState *s;
    u8 *code_buffer;
    u8 bitmap[BITMAP_SIZE];
    u64 next;
    u64 free_units;
} CodeGenerator;

CodeGenerator *codegen_create(void);
void codegen_destroy(CodeGenerator *cg);
void codegen_compile(CodeGenerator *cg, CacheEntry *entry);
u8 *codegen_alloc(CodeGenerator *cg, size_t length);
void codegen_free(CodeGenerator *cg, const u8 *code, size_t length);

#endif // CODEGEN_H
