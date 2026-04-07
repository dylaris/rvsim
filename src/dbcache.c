#include "dbcache.h"
#include "memory.h"

DBCache dbcache_create(u64 size)
{
    return (DBCache) {
        .block_lookup = hash_create(size),
        .blocks = NULL,
        .hot_lookup = hash_create(MB(2)),
        .hots = NULL,
    };
}

void dbcache_destroy(DBCache *cache)
{
    hash_destroy(&cache->block_lookup);
    array_free(cache->blocks);
    hash_destroy(&cache->hot_lookup);
    array_free(cache->hots);
}

bool dbcache_hot(DBCache *cache, u64 pc)
{
    u64 invalid_pc = 0;
    u32 value = hash_get(&cache->hot_lookup, pc, invalid_pc);
    if (value == invalid_pc || value < DBCACHE_HOT)
        return false;
    return true;
}

BasicBlock *dbcache_lookup(DBCache *cache, u64 pc)
{
    u64 invalid_pc = 0;
    u32 value;

    value = hash_get(&cache->hot_lookup, pc, invalid_pc);
    if (value == invalid_pc) {
        if (hash_full(&cache->hot_lookup))
            hash_grow(&cache->hot_lookup, cache->hot_lookup.capacity * 2);
        hash_set(&cache->hot_lookup, pc, array_len(cache->hots));
        array_push(cache->hots, 1);
    } else
        cache->hots[value] += 1;

    value = hash_get(&cache->block_lookup, pc, invalid_pc);
    if (value == invalid_pc)
        return NULL;
    return &cache->blocks[value];
}

static inline bool is_block_end(InstrKind kind)
{
    switch (kind) {
    case INSTR_beq:
    case INSTR_bne:
    case INSTR_blt:
    case INSTR_bge:
    case INSTR_bltu:
    case INSTR_bgeu:
    case INSTR_jalr:
    case INSTR_jal:
    case INSTR_ecall:
        return true;
    default:
        return false;
    }
}

BasicBlock dbcache_compile(u64 pc)
{
    Instr instr = {0};
    BasicBlock block = { .pc = pc, .instrs = NULL };

    while (true) {
        u32 raw = mem_read_u32(pc);
        assert(decode_instr(raw, &instr));
        array_push(block.instrs, instr);
        if (is_block_end(instr.kind))
            break;
        pc += instr.rvc ? 2 : 4;
    }

    return block;
}

BasicBlock *dbcache_add(DBCache *cache, BasicBlock block)
{
    if (!hash_set(&cache->block_lookup, block.pc, array_len(cache->blocks)))
        return NULL;
    array_push(cache->blocks, block);
    return array_tail(cache->blocks);
}

