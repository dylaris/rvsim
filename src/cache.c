#include "cache.h"

#include <sys/mman.h>

#define sys_icache_invalidate(addr, size) \
  __builtin___clear_cache((char *)(addr), (char *)(addr) + (size));

Cache cache_create(u64 capacity)
{
    Cache cache = {
        .jitcode = (u8 *) mmap(NULL, capacity, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0),
        .lookup = (CacheLookup) {0},
        .offset = 0,
        .capacity = capacity,
    };
    return cache;
}

void cache_destroy(Cache *cache)
{
    ht_free(&cache->lookup);
    munmap(cache->jitcode, cache->capacity);
}

CacheEntry *cache_lookup(Cache *cache, u64 pc)
{
    CacheEntry *entry = ht_find(&cache->lookup, pc);
    if (entry)
        entry->hot++;
    else
        *ht_put(&cache->lookup, pc) = (CacheEntry) { .pc = pc, .hot = 1 };
    return ht_find(&cache->lookup, pc);
}

static __ForceInline u64 align_to(u64 val, u64 align)
{
    if (align == 0)
        return val;
    return (val + align - 1) & ~(align - 1);
}

u8 *cache_add(Cache *cache, u64 pc, u8 *code, size_t sz, u64 align)
{
    cache->offset = align_to(cache->offset, align);
    assert(cache->offset + sz <= CACHE_SIZE);

    CacheEntry *entry = ht_find(&cache->lookup, pc);

    memcpy(cache->jitcode + cache->offset, code, sz);
    entry->pc = pc;
    entry->offset = cache->offset;
    entry->length = sz;
    entry->code = cache->jitcode + entry->offset;
    cache->offset += sz;
    sys_icache_invalidate(entry->code, sz);
    return entry->code;
}
