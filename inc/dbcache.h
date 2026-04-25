// Decode Block Cache

#ifndef DBCACHE_H
#define DBCACHE_H

#include "decode.h"

#define DBCACHE_SIZE 4096
#define DBCACHE_MAX_PROBE_STEPS DBCACHE_SIZE

typedef struct {
    u64 pc;
    u64 next_pc;
    Instr *items;
    size_t count;
    size_t capacity;
} DBCacheEntry;

typedef struct {
    DBCacheEntry table[DBCACHE_SIZE];
    DBCacheEntry *last_accessed;
    bool full;
} DBCache;

DBCache *dbcache_create(void);
void dbcache_destroy(DBCache *cache);
DBCacheEntry *dbcache_lookup(DBCache *cache, u64 pc);
DBCacheEntry *dbcache_add(DBCache *cache, u64 pc);

#endif // DBCACHE_H
