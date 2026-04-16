#include "profile.h"
#include "ht.h"

bool block_should_link(u64 pc, u64 target_addr)
{
    u64 range = pc > target_addr ? pc - target_addr : target_addr - pc;
    if (range <= SMALL_BRANCH_RANGE) return true;
    if (bb_count(target_addr) >= HOT_PATH_THRESHOLD) return true;
    return false;
}

static Ht(u64, u64) bb_hit_counts = {0};

u64 bb_hit(u64 pc)
{
    u64 *count = ht_find(&bb_hit_counts, pc);
    if (count) {
        *count += 1;
        return *count;
    } else {
        *ht_put(&bb_hit_counts, pc) = 1;
        return 1;
    }
}

u64 bb_count(u64 pc)
{
    u64 *count = ht_find(&bb_hit_counts, pc);
    if (count) return *count;
    else return 0;
}
