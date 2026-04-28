#ifndef CACHE_H
#define CACHE_H

#include "decode.h"
#include "codegen.h"

#define CACHE_SIZE 4096
#define CACHE_HOT_COUNT 10000
#define CACHE_MAX_PROBE_STEPS CACHE_SIZE
#define DYN_LINK_CACHE_SIZE 16

typedef struct CacheEntry CacheEntry;
struct CacheEntry {
    u64 pc;

    // Decode block cache
    Instr *items;
    size_t count;
    size_t capacity;

    // Translate block cache
    u8 *code;
    size_t code_length;

    // LRU
    u64 last_access_timestamp;

    // Hot profile
    u64 access_count;

    // Link
    union {
        // static jump - branch
        struct {
            CacheEntry *branch_taken;
            CacheEntry *branch_not_taken;
        };

        // static jump - jal
        CacheEntry *jal_target;

        // dynamic jump - jalr
        struct {
            struct {
                u64 target_pc;
                CacheEntry *target_entry;
            } dyn_link_cache[DYN_LINK_CACHE_SIZE];
            int dyn_link_next;
        };
    };
};

typedef struct {
    CacheEntry table[CACHE_SIZE];
    CacheEntry *last;
    u64 timestamp_counter;
    CodeGenerator *cg;
} Cache;

Cache *cache_create(void);
void cache_destroy(Cache *cache);
CacheEntry *cache_get(Cache *cache, u64 pc);
void cache_touch(Cache *cache, CacheEntry *entry);

#define sys_icache_invalidate(addr, size) \
    do { \
        if (addr) { \
            __builtin___clear_cache((char *)(addr), (char *)(addr) + (size)); \
        } \
    } while (0)

#endif // CACHE_H
