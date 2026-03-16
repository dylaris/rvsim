#include "mmu.h"
#include "elf64.h"

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
}
