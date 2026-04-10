#ifndef CACHE_H
#define CACHE_H

#include "common.h"

#define CACHE_HOT_COUNT 10000

typedef struct {
    u64 pc;
    u64 hot;
    u64 instr_count;
    u64 offset;
    u64 length;
} CacheEntry;

typedef struct {
    u8 *jitcode;
    u64 end;
    u64 capacity;
    Hash entry_lookup;
    CacheEntry __Array *entries;
} Cache;

Cache cache_create(u64 capacity, u64 nentry);
void cache_destroy(Cache *cache);
u64 cache_lookup(Cache *cache, u64 pc);

static __ForceInline __Keep u8 *cache_code(Cache *cache, u64 index)
{
    u64 instr_count = cache->entries[index].instr_count;
    if (instr_count == 0)
        return NULL;
    return cache->jitcode + cache->entries[index].offset;
}

static __ForceInline __Keep bool cache_hot(Cache *cache, u64 index)
{
    return cache->entries[index].hot >= CACHE_HOT_COUNT;
}

#endif // CACHE_H
