#include "cache.h"

#include <sys/mman.h>

#define sys_icache_invalidate(addr, size) \
  __builtin___clear_cache((char *)(addr), (char *)(addr) + (size));


static uintptr_t hasheq(Ht_Op op, void const* a_, void const *b_, size_t n)
{
    u64 a = *(u64 *) a_;
    u64 b = *(u64 *) b_;
    switch (op) {
    case HT_HASH: return ht_default_hash((void *) &a, n);
    case HT_EQ:   return a == b;
    }
    return 0;
}

Cache *cache_create(u64 capacity)
{
    Cache *cache = malloc(sizeof(Cache));
    assert(cache && "run out of memory");

    cache->jitcode = (u8 *) mmap(NULL, capacity, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0),
    cache->lookup = (CacheLookup) { .hasheq = hasheq },
    cache->offset = 0,
    cache->capacity = capacity,
    pthread_mutex_init(&cache->mutex, NULL);

    return cache;
}

void cache_destroy(Cache *cache)
{
    pthread_mutex_lock(&cache->mutex);
    ht_free(&cache->lookup);
    munmap(cache->jitcode, cache->capacity);
    pthread_mutex_unlock(&cache->mutex);

    pthread_mutex_destroy(&cache->mutex);
    free(cache);
}

CacheEntry *cache_lookup(Cache *cache, u64 pc)
{
    pthread_mutex_lock(&cache->mutex);

    CacheEntry *entry = ht_find(&cache->lookup, pc);
    if (entry)
        entry->hot++;
    else {
        *ht_put(&cache->lookup, pc) = (CacheEntry) { .pc = pc, .hot = 1, .gen = false };
        entry = ht_find(&cache->lookup, pc);
    }

    pthread_mutex_unlock(&cache->mutex);

    return entry;
}

static __ForceInline u64 align_to(u64 val, u64 align)
{
    if (align == 0)
        return val;
    return (val + align - 1) & ~(align - 1);
}

u8 *cache_add(Cache *cache, u64 pc, u8 *code, size_t sz, u64 align)
{
    pthread_mutex_lock(&cache->mutex);

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

    pthread_mutex_unlock(&cache->mutex);

    return entry->code;
}
