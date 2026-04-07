#ifndef DBCACHE_H
#define DBCACHE_H

#include "minicc.h"
#include "decode.h"

#define DBCACHE_HOT 1000

typedef struct {
    u64 pc;
    Instr __Array *instrs;
} BasicBlock;

typedef struct {
    Hash block_lookup;
    BasicBlock __Array *blocks;

    Hash hot_lookup;
    u64 __Array *hots;
} DBCache;

DBCache dbcache_create(u64 size);
void dbcache_destroy(DBCache *cache);
BasicBlock *dbcache_lookup(DBCache *cache, u64 pc);
BasicBlock dbcache_compile(u64 pc);
bool dbcache_hot(DBCache *cache, u64 pc);
BasicBlock *dbcache_add(DBCache *cache, BasicBlock block);

#endif // DBCACHE_H
