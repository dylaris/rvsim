#include "dbcache.h"
#include "memory.h"
#include "nob.h"

DBCache *dbcache_create(void)
{
    DBCache *cache = calloc(1, sizeof(DBCache));
    cache->full = false;
    assert(cache && "Out of memory");
    return cache;
}

void dbcache_destroy(DBCache *cache)
{
    if (!cache) return;
    for (int i = 0; i < DBCACHE_SIZE; i++) {
        DBCacheEntry *entry = &cache->table[i];
        if (entry->pc != 0) da_free(*entry);
    }
    free(cache);
}

#define dbcache__hash(pc_) ((pc_) & (DBCACHE_SIZE - 1))

DBCacheEntry *dbcache_lookup(DBCache *cache, u64 pc)
{
    if (cache->last_accessed != NULL && cache->last_accessed->pc == pc) {
        return cache->last_accessed;
    }

    size_t index = dbcache__hash(pc);

    for (int step = 0; step < DBCACHE_MAX_PROBE_STEPS; step++) {
        DBCacheEntry *entry = &cache->table[index];

        if (entry->pc == pc) {
            cache->last_accessed = entry;
            return entry;
        }

        if (entry->pc == 0) break;

        index = (index + 1) & (DBCACHE_SIZE - 1);
    }

    return NULL;
}

static inline bool is_block_end(InstrKind kind)
{
    switch (kind) {
    case instr_beq: case instr_bne:
    case instr_blt: case instr_bge:
    case instr_bltu: case instr_bgeu:
    case instr_jal:
    case instr_jalr:
    case instr_ecall:
        return true;
    default:
        return false;
    }
}

DBCacheEntry *dbcache_add(DBCache *cache, u64 pc)
{
    size_t index = dbcache__hash(pc);

    for (int step = 0; step < DBCACHE_MAX_PROBE_STEPS; step++) {
        DBCacheEntry *entry = &cache->table[index];

        if (entry->pc == 0) {
            entry->pc = pc;

            u64 curr_pc = pc;
            while (true) {
                Instr instr = {0};
                decode_instr(curr_pc, &instr);
                da_append(entry, instr);

                if (is_block_end(instr.kind)) break;
                curr_pc += instr.rvc ? 2 : 4;
            }

            cache->last_accessed = entry;
            return entry;
        }

        if (entry->pc == pc) {
            cache->last_accessed = entry;
            return entry;
        }

        index = (index + 1) & (DBCACHE_SIZE - 1);
    }

    cache->full = true;
    return NULL;
}

