#ifndef CODEGEN_H
#define CODEGEN_H

#include "common.h"
#include "ht.h"
#include "nob.h"

typedef Ht(u64, bool) Set;

typedef struct {
    String_Builder sb;
    Set set;
    Stack links;
} CodeGenerator;

CodeGenerator *codegen_create(void);
void codegen_destroy(CodeGenerator *cg);
void *gencode(CodeGenerator *cg, TBCache *cache, u64 start_pc);

#endif // CODEGEN_H
