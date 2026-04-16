#include "dbcache.h"
#include "memory.h"
#include "nob.h"

DBCache *dbcache_create(size_t n)
{
    DBCache *cache = malloc(sizeof(DBCache));
    assert(cache && "run out of memory");
    cache->items = malloc(sizeof(DBCacheEntry) * n);
    cache->count = 0;
    cache->capacity = n;
    return cache;
}

void dbcache_destroy(DBCache *cache)
{
    da_foreach(DBCacheEntry, entry, cache) {
        da_free(*entry);
    }
    da_free(*cache);
    free(cache);
}

DBCacheEntry *dbcache_lookup(DBCache *cache, u64 pc)
{
    da_foreach(DBCacheEntry, entry, cache) {
        if (entry->pc == pc) return entry;
    }
    return NULL;
}

static inline bool is_block_end(InstrKind kind)
{
    switch (kind) {
    case instr_beq:
    case instr_bne:
    case instr_blt:
    case instr_bge:
    case instr_bltu:
    case instr_bgeu:
    case instr_jalr:
    case instr_jal:
    case instr_ecall:
        return true;
    default:
        return false;
    }
}

DBCacheEntry *dbcache_add(DBCache *cache, u64 pc)
{
    DBCacheEntry entry = { .pc = pc };
    while (true) {
        Instr instr = {0};
        u32 raw = mem_read_u32(pc);
        assert(decode_instr(raw, &instr));
        da_append(&entry, instr);
        if (is_block_end(instr.kind)) break;
        pc += instr.rvc ? 2 : 4;
    }
    da_append(cache, entry);
    return &cache->items[cache->count - 1];
}

