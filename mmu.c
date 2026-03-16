#include "mmu.h"
#include "elf64.h"

#include <stdio.h>

static void mmu__load_segment(MMU *mmup, ProgHeader *phdrp, int fd)
{
    UNUSED(mmup);
    UNUSED(phdrp);
    UNUSED(fd);
}

void mmu_load_elf(MMU *mmup, FILE *fp)
{
    ELFHeader ehdr;

    // Read ELF header
    if (fread((void *) &ehdr, sizeof(ELFHeader), 1, fp) != 1)
        fatal("file too small");

    // Check magic number
    if (*(u32 *) &ehdr != *(u32 *) ELFMAG)
        fatal("bad elf file");

    // Check architecture
    if (ehdr.machine != EM_RISCV || ehdr.ident[EI_CLASS] != ELFCLASS64)
        fatal("only support riscv64 elf file");

    // Set entry point
    mmup->entry = ehdr.entry;

    // Load segment
    for (u64 i = 0; i < (u64) ehdr.phnum; i++) {
        u64 offset = ehdr.phoff + i * sizeof(ProgHeader);
        if (fseek(fp, (long) offset, SEEK_SET) != 0)
            fatalf("seek file failed when loading No.%ld program header", i);

        ProgHeader phdr;
        if (fread((void *) &phdr, sizeof(ProgHeader), 1, fp) != 1)
            fatal("file tool small");

        if (phdr.type == PT_LOAD)
            mmu__load_segment(mmup, &phdr, fileno(fp));
    }
}
