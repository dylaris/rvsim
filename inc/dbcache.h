// Decode Block Cache

#ifndef DBCACHE_H
#define DBCACHE_H

#include "decode.h"

#define DBCACHE_HOT_COUNT 10000

typedef struct {
    u64 pc;
    Instr *items;
    size_t count;
    size_t capacity;
} DBCacheEntry;

typedef struct {
    DBCacheEntry *items;
    size_t count;
    size_t capacity;
    DBCacheEntry *last_accessed;
} DBCache;

DBCache *dbcache_create(size_t n);
void dbcache_destroy(DBCache *cache);
DBCacheEntry *dbcache_lookup(DBCache *cache, u64 pc);
DBCacheEntry *dbcache_add(DBCache *cache, u64 pc);

#endif // DBCACHE_H
