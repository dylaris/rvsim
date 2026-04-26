// Decode Block Cache

#ifndef DBCACHE_H
#define DBCACHE_H

#include "decode.h"

#define DBCACHE_SIZE 4096
#define DBCACHE_MAX_PROBE_STEPS DBCACHE_SIZE
#define DYN_LINK_CACHE_SIZE 16

typedef struct DBCacheEntry DBCacheEntry;
struct DBCacheEntry {
    Instr *items;
    size_t count;
    size_t capacity;

    u64 pc;

    // LRU
    u64 last_access_timestamp;
    u64 access_count;

    union {
        // static jump - branch
        struct {
            DBCacheEntry *branch_taken;
            DBCacheEntry *branch_not_taken;
        };

        // static jump - jal
        DBCacheEntry *jal_target;

        // dynamic jump - jalr
        struct {
            struct {
                u64 target_pc;
                DBCacheEntry *target_entry;
            } dyn_link_cache[DYN_LINK_CACHE_SIZE];
            int dyn_link_next;
        };
    };
};

typedef struct {
    DBCacheEntry table[DBCACHE_SIZE];
    DBCacheEntry *last;
    u64 timestamp_counter;
} DBCache;

DBCache *dbcache_create(void);
void dbcache_destroy(DBCache *cache);
DBCacheEntry *dbcache_get(DBCache *cache, u64 pc);

#endif // DBCACHE_H
