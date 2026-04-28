#include "cache.h"
#include "memory.h"
#include "nob.h"

#define sys_icache_invalidate(addr, size) \
    __builtin___clear_cache((char *)(addr), (char *)(addr) + (size)); \

Cache *cache_create(void)
{
    Cache *cache = malloc(sizeof(Cache));
    assert(cache && "Out of memory");
    memset(cache, 0, sizeof(Cache));
    // cache->cg = codegen_create();
    return cache;
}

void cache_destroy(Cache *cache)
{
    if (!cache) return;
    for (int i = 0; i < CACHE_SIZE; i++) {
        CacheEntry *entry = &cache->table[i];
        if (entry->pc != 0) da_free(*entry);
    }
    // codegen_destroy(cache->cg);
    free(cache);
}

#define cache__hash(pc_) ((pc_) & (CACHE_SIZE - 1))

void cache_touch(Cache *cache, CacheEntry *entry)
{
    entry->last_access_timestamp = ++cache->timestamp_counter;
    entry->access_count++;
    cache->last = entry;

    // if (entry->access_count >= CACHE_HOT_COUNT && !entry->code) {
    //     codegen_compile(cache->cg, entry);
    //     if (entry->code) {
    //         sys_icache_invalidate(entry->code, entry->code_length);
    //     }
    // }
}

static void cache__reset_entry(Cache *cache, CacheEntry *entry)
{
    // if (entry->code) codegen_free(cache->cg, entry->code, entry->code_length);
    Instr *saved_items = entry->items;
    size_t saved_capacity = entry->capacity;
    memset(entry, 0, sizeof(CacheEntry));
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

static CacheEntry *cache__find_evict_entry(Cache *cache)
{
    size_t evict_index = 0;
    u64 min_timestamp = cache->table[evict_index].last_access_timestamp;

    for (size_t i = 1; i < CACHE_SIZE; i++) {
        CacheEntry *entry = &cache->table[i];
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

CacheEntry *cache_get(Cache *cache, u64 pc)
{
    assert(pc != 0);

    if (cache->last && cache->last->pc == pc) {
        cache_touch(cache, cache->last);
        return cache->last;
    }

    size_t index = cache__hash(pc);
    CacheEntry *entry = NULL;

    // Find empty entry or exist entry
    for (int step = 0; step < CACHE_MAX_PROBE_STEPS; step++) {
        entry = &cache->table[index];

        if (entry->pc == 0) goto fill_entry;

        if (entry->pc == pc) {
            cache_touch(cache, entry);
            return entry;
        }

        index = (index + 1) & (CACHE_SIZE - 1);
    }

    // Cache is full, evict the oldest entry
    entry = cache__find_evict_entry(cache);
    cache__reset_entry(cache, entry);

fill_entry:
    // Empty entry or evict entry (need to reset)
    entry->pc = pc;
    cache_touch(cache, entry);
    u64 curr_pc = pc;
    while (true) {
        Instr instr = {0};
        decode_instr(curr_pc, &instr);
        da_append(entry, instr);
        if (instr.cfc) break;
        curr_pc += instr.rvc ? 2 : 4;
    }

    return entry;
}

void cache_print(const CacheEntry *entry)
{
    da_foreach(Instr, instr, entry) {
        printf("[%lx] %s\n", instr->curr_pc, instr_to_string(instr));
    }
}
