// Translate Block Cache

#ifndef TBCACHE_H
#define TBCACHE_H

#include "common.h"
#include "ht.h"

#define TBCACHE_HOT_COUNT 10000
#define TBCACHE_BLOCK_CAPACITY MB(1)

typedef struct {
    u64 pc;
    void *code;
} TBCacheEntry;

typedef Ht(u64, TBCacheEntry) TBCacheLookup;

typedef struct {
    u8 *buffer;
    size_t used;
} TBCacheBlock;

typedef struct {
    TBCacheBlock *items;
    size_t count;
    size_t capacity;
    TBCacheLookup lookup;
} TBCache;

TBCache *tbcache_create(size_t nblock);
void tbcache_destroy(TBCache *cache);
TBCacheEntry *tbcache_lookup(TBCache *cache, u64 pc);
void *tbcache_add(TBCache *cache, void *code, size_t length);

#endif // CACHE_H
