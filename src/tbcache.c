#include "tbcache.h"

#define sys_icache_invalidate(addr, size) \
  __builtin___clear_cache((char *)(addr), (char *)(addr) + (size));

static __ForceInline TBCacheBlock tbcache__new_block(void)
{
    return (TBCacheBlock) {
        .buffer = mmap(NULL, TBCACHE_BLOCK_CAPACITY,
                       PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0),
        .used = 0,
    };
}

TBCache *tbcache_create(size_t nblock)
{
    TBCache *cache = malloc(sizeof(TBCache));
    assert(cache && "run out of memory");
    memset(cache, 0, sizeof(TBCache));

    for (size_t i = 0; i < nblock; i++) {
        TBCacheBlock block = tbcache__new_block();
        da_append(cache, block);
    }

    return cache;
}

void tbcache_destroy(TBCache *cache)
{
    da_foreach(TBCacheBlock, block, cache) {
        munmap(block->buffer, TBCACHE_BLOCK_CAPACITY);
    }
    da_free(*cache);
    ht_free(&cache->lookup);
    free(cache);
}

TBCacheEntry *tbcache_lookup(TBCache *cache, u64 pc)
{
    TBCacheEntry *entry = ht_find(&cache->lookup, pc);
    if (!entry) {
        *ht_put(&cache->lookup, pc) = (TBCacheEntry) { .pc = pc, .code = NULL };
        entry = ht_find(&cache->lookup, pc);
    }
    return entry;
}

static TBCacheBlock *tbcache__find_block(TBCache *cache, size_t length)
{
    da_foreach(TBCacheBlock, block, cache) {
        if (length <= TBCACHE_BLOCK_CAPACITY - block->used) return block;
    }
    TBCacheBlock block = tbcache__new_block();
    da_append(cache, block);
    return &cache->items[cache->count - 1];
}

void *tbcache_add(TBCache *cache, void *code, size_t length)
{
    assert(length < TBCACHE_BLOCK_CAPACITY && "cache block overflow");
    TBCacheBlock *block = tbcache__find_block(cache, length);
    void *dst = block->buffer + block->used;
    memcpy(dst, code, length);
    sys_icache_invalidate(dst, length);
    block->used += length;
    return dst;
}
