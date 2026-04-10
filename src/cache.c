#include "cache.h"

#include <sys/mman.h>

#define sys_icache_invalidate(addr, size) \
  __builtin___clear_cache((char *)(addr), (char *)(addr) + (size));

Cache cache_create(u64 capacity, u64 nentry)
{
    Cache cache = {
        .jitcode = (u8 *) mmap(NULL, capacity, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0),
        .entry_lookup = hash_create(nentry),
        .entries = NULL,
        .end = 0,
        .capacity = capacity,
    };
    array_ensure(cache.entries, nentry);
    return cache;
}

void cache_destroy(Cache *cache)
{
    hash_destroy(&cache->entry_lookup);
    array_free(cache->entries);
    munmap(cache->jitcode, cache->capacity);
}

u64 cache_lookup(Cache *cache, u64 pc)
{
    u64 value = hash_get(&cache->entry_lookup, pc, -1ULL);
    if (value == -1ULL) {
        if (hash_factor(&cache->entry_lookup) >= HASH_LOAD_FACTOR)
            hash_grow(&cache->entry_lookup, 2*cache->entry_lookup.capacity);
        value = array_len(cache->entries);
        hash_set(&cache->entry_lookup, pc, value);
        array_push(cache->entries, (CacheEntry) { .pc = pc });
    }
    cache->entries[value].hot++;
    return value;
}
