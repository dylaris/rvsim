#include "memory.h"
#include "elf64.h"

#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

static void mem__update_in_loading(Memory *mem, HostVAddr vaddr, u64 size)
{
    u64 page_size = getpagesize();
    if (mem->host_begin == mem->host_end) mem->host_begin = vaddr;
    mem->host_end = MAX(mem->host_end, vaddr + ROUNDUP(size, page_size));
    mem->heap_base = mem->heap_brk = mem->heap_end = mmu_to_guest(mem->host_end);
}

static void mem__load_segment(Memory *mem, ProgHeader *phdr, int fd)
{
    u64       page_size     = getpagesize();
    u64       offset        = phdr->offset;
    HostVAddr vaddr         = mmu_to_host(phdr->vaddr);
    HostVAddr aligned_vaddr = ROUNDDOWN(vaddr, page_size);
    u64       mem_size      = phdr->memsz + (vaddr - aligned_vaddr);
    u64       file_size     = phdr->filesz + (vaddr - aligned_vaddr);

    int mmap_prot = 0;
    if (phdr->flags & PF_R) mmap_prot |= PROT_READ;
    if (phdr->flags & PF_W) mmap_prot |= PROT_WRITE;
    if (phdr->flags & PF_X) mmap_prot |= PROT_EXEC;

    if (mmap((void *) aligned_vaddr, mem_size, mmap_prot,
             MAP_FIXED | MAP_PRIVATE, fd, ROUNDDOWN(offset, page_size)) == MAP_FAILED) {
        fatalf("mmap: %s", strerror(errno));
    }
    mem__update_in_loading(mem, aligned_vaddr, mem_size);

    u64 remaining_bss = ROUNDUP(mem_size, page_size) - ROUNDUP(file_size, page_size);
    if (remaining_bss > 0) {
        if (mmap((void *) (aligned_vaddr + ROUNDUP(file_size, page_size)), remaining_bss,
                 mmap_prot, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
            fatalf("mmap: %s", strerror(errno));
        }
    }
    mem__update_in_loading(mem, aligned_vaddr + ROUNDUP(file_size, page_size), remaining_bss);
}

void mem_load_elf(Memory *mem, FILE *f)
{
    ELFHeader ehdr;

    // Read ELF header
    if (fread((void *) &ehdr, sizeof(ELFHeader), 1, f) != 1) {
        fatalf("fread: %s", strerror(errno));
    }

    // Check magic number
    if (*(u32 *) &ehdr != *(u32 *) ELFMAG) {
        fatal("bad elf file");
    }

    // Check architecture
    if (ehdr.machine != EM_RISCV || ehdr.ident[EI_CLASS] != ELFCLASS64) {
        fatal("only support riscv64 elf file");
    }

    // Set entry point
    mem->entry = (GuestVAddr) ehdr.entry;

    // Load segment
    for (u64 i = 0; i < (u64) ehdr.phnum; i++) {
        u64 offset = ehdr.phoff + i * sizeof(ProgHeader);
        if (fseek(f, (long) offset, SEEK_SET) != 0) {
            fatalf("fseek: %s", strerror(errno));
        }

        ProgHeader phdr;
        if (fread((void *) &phdr, sizeof(ProgHeader), 1, f) != 1) {
            fatalf("fread: %s", strerror(errno));
        }

        if (phdr.type == PT_LOAD) {
            mem__load_segment(mem, &phdr, fileno(f));
        }
    }
}

void mem_load_bin(Memory *mem, FILE *f, GuestVAddr base)
{
    mem->entry = base;

    fseek(f, 0, SEEK_END);
    u64 file_size = ftell(f);
    rewind(f);

    u64 page_size = getpagesize();
    HostVAddr vaddr = mmu_to_host(base);
    HostVAddr aligned_vaddr = ROUNDDOWN(vaddr, page_size);
    u64 mem_size = file_size + (aligned_vaddr - vaddr);

    if (mmap((void *) aligned_vaddr, mem_size,
             PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_PRIVATE, fileno(f), 0) == MAP_FAILED) {
        fatalf("mmap: %s", strerror(errno));
    }
    mem__update_in_loading(mem, aligned_vaddr, mem_size);

    if (fread((void *) vaddr, 1, file_size, f) != file_size) {
        fatalf("fread: %s", strerror(errno));
    }
}

GuestVAddr mem_alloc(Memory *mem, i64 size)
{
    GuestVAddr res = mem->heap_brk;
    u64 page_size = getpagesize();

    GuestVAddr new_brk = mem->heap_brk + size;
    if (new_brk < mem->heap_base) fatal("heap underflow (brk < base)");
    mem->heap_brk = new_brk;

    if (size > 0) {
        if (mem->heap_brk > mem->heap_end) {
            if (mmap((void *) mem->host_end, ROUNDUP(size, page_size),
                     PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0) == MAP_FAILED) {
                fatalf("mmap: %s", strerror(errno));
            }
            mem->host_end += ROUNDUP(size, page_size);
        }
    } else if (size < 0) {
        if (ROUNDUP(mem->heap_brk, page_size) < mem->heap_end) {
            u64 len = mem->heap_end - ROUNDUP(mem->heap_brk, page_size);
            if (munmap((void *) mem->host_end, len) == -1) {
                fatalf("munmap: %s", strerror(errno));
            }
            mem->host_end -= len;
        }
    }

    mem->heap_end = mmu_to_guest(mem->host_end);

    return res;
}

void mem_free(Memory *mem)
{
    u64 len = mem->host_end - mem->host_begin;
    if (len > 0) munmap((void *) mem->host_begin, len);
    memset(mem, 0, sizeof(*mem));
}
