#include "dbcache.h"
#include "memory.h"
#include "nob.h"

DBCache *dbcache_create(void)
{
    DBCache *cache = malloc(sizeof(DBCache));
    assert(cache && "Out of memory");
    memset(cache, 0, sizeof(DBCache));
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

static void dbcache__update_access(DBCache *cache, DBCacheEntry *entry)
{
    entry->last_access_timestamp = ++cache->timestamp_counter;
    entry->access_count++;
    cache->last = entry;
}

static void dbcache__reset_entry(DBCacheEntry *entry)
{
    Instr *saved_items = entry->items;
    size_t saved_capacity = entry->capacity;
    memset(entry, 0, sizeof(DBCacheEntry));
    entry->items = saved_items;
    entry->capacity = saved_capacity;
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

static DBCacheEntry *dbcache__find_evict_entry(DBCache *cache)
{
    size_t evict_index = 0;
    u64 min_timestamp = cache->table[evict_index].last_access_timestamp;

    for (size_t i = 1; i < DBCACHE_SIZE; i++) {
        DBCacheEntry *entry = &cache->table[i];
        // Prefer empty entry
        if (entry->pc == 0) {
            evict_index = i;
            break;
        }
        if (entry->last_access_timestamp < min_timestamp) {
            min_timestamp = entry->last_access_timestamp;
            evict_index = i;
        }
    }
    return &cache->table[evict_index];
}

DBCacheEntry *dbcache_get(DBCache *cache, u64 pc)
{
    assert(pc != 0);

    if (cache->last && cache->last->pc == pc) {
        dbcache__update_access(cache, cache->last);
        return cache->last;
    }

    size_t index = dbcache__hash(pc);
    DBCacheEntry *entry = NULL;

    // Find empty entry or exist entry
    for (int step = 0; step < DBCACHE_MAX_PROBE_STEPS; step++) {
        entry = &cache->table[index];

        if (entry->pc == 0) goto fill_entry;

        if (entry->pc == pc) {
            dbcache__update_access(cache, entry);
            return entry;
        }

        index = (index + 1) & (DBCACHE_SIZE - 1);
    }

    // Cache is full, evict the oldest entry
    entry = dbcache__find_evict_entry(cache);
    dbcache__reset_entry(entry);

fill_entry:
    // Empty entry or evict entry (need to reset)
    entry->pc = pc;
    dbcache__update_access(cache, entry);
    u64 curr_pc = pc;
    while (true) {
        Instr instr = {0};
        decode_instr(curr_pc, &instr);
        da_append(entry, instr);
        if (is_block_end(instr.kind)) break;
        curr_pc += instr.rvc ? 2 : 4;
    }

    return entry;
}
