#ifndef CACHE_H
#define CACHE_H

#include "common.h"

#define CACHE_HOT_COUNT 10000

typedef struct {
    u64 pc;
    u64 hot;
    u64 offset;
} CacheEntry;

typedef struct {
    u8 *jitcode;
    u64 offset;
    Hash entry_lookup;
    CacheEntry __Array *entries;
    u64 capacity;
} Cache;

Cache cache_create(u64 size);
void cache_destrooy(Cache *cache);
u8 *cache_lookup(Cache *cache, u64 pc);
u8 *cache_add(Cache *cache, u64 pc, u8 *, size_t, u64);
#define cache_is_hot(c, i) ((c)->entries[i].hot >= CACHE_HOT_COUNT)

#endif // CACHE_H
