#include "machine.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

void machine_load_program(Machine *mp, const char *prog)
{
    FILE *fp = fopen(prog, "rb");
    if (!fp)
        fatal(strerror(errno));

    mmu_load_elf(&mp->mmu, fp);

    fclose(fp);
}
