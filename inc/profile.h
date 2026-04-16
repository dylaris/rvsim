#ifndef PROFILE_H
#define PROFILE_H

#include "common.h"
#include "tbcache.h"

#define SMALL_BRANCH_RANGE 1024
#define HOT_PATH_THRESHOLD (TBCACHE_HOT_COUNT / 20)

bool block_should_link(u64 pc, u64 target_addr);
u64 bb_hit(u64 pc);
u64 bb_count(u64 pc);

#endif // PROFILE_H
