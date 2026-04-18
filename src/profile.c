#include "profile.h"
#include "ht.h"

bool profile_block_should_link(u64 pc, u64 target_addr)
{
    u64 range = pc > target_addr ? pc - target_addr : target_addr - pc;
    if (range <= SMALL_BRANCH_RANGE) return true;
    if (profile_block_count(target_addr) >= HOT_PATH_THRESHOLD) return true;
    return false;
}

static Ht(u64, u64) block_hit_counts = {0};

u64 profile_block_hit(u64 pc)
{
    u64 *count = ht_find(&block_hit_counts, pc);
    if (count) {
        *count += 1;
        return *count;
    } else {
        *ht_put(&block_hit_counts, pc) = 1;
        return 1;
    }
}

u64 profile_block_count(u64 pc)
{
    u64 *count = ht_find(&block_hit_counts, pc);
    if (count) return *count;
    else return 0;
}

void profile_free(void)
{
    ht_free(&block_hit_counts);
}
