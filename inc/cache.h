#ifndef CACHE_H
#define CACHE_H

#include "common.h"
#include "ht.h"

#define CACHE_HOT_COUNT 10000

#define CACHE_SIZE MB(16)

typedef struct {
    u64 pc;
    u64 hot;
    u8 *code;
    u64 offset;
    u64 length;
} CacheEntry;

typedef Ht(u64, CacheEntry) CacheLookup;

typedef struct {
    u8 *jitcode;
    u64 offset;
    CacheLookup lookup;
    u64 capacity;
} Cache;

Cache cache_create(u64 capacity);
void cache_destroy(Cache *cache);
CacheEntry *cache_lookup(Cache *cache, u64 pc);
u8 *cache_add(Cache *cache, u64 pc, u8 *code, size_t sz, u64 align);

#endif // CACHE_H
