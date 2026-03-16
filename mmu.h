#ifndef MMU_H
#define MMU_H

#include "utils.h"

#include <stdio.h>

typedef struct {
    u64 entry;
} MMU;

void mmu_load_elf(MMU *mmup, FILE *fp);

#endif // MMU_H
