#ifndef MEMORY_H
#define MEMORY_H

#include "mmu.h"

#include "string.h"

// host_end = TO_HOST(heap_end)
// heap_end = TO_GUEST(host_end)
typedef struct {
    GuestVAddr entry;       // ELF entry point
    HostVAddr  host_end;    // Host mapped region end address
    GuestVAddr heap_base;   // Guest heap base address
    GuestVAddr heap_brk;    // Guest heap program break
    GuestVAddr heap_end;    // Guest heap end address
} Memory;

void mem_load_elf(Memory *mem, FILE *f);
GuestVAddr mem_alloc(Memory *mem, i64 size);

static inline void mem_write(GuestVAddr addr, void *data, u64 len)
{
    memcpy((void *) mmu_to_host(addr), data, len);
}

static inline u8 mem_read_byte(GuestVAddr addr)
{
    return *(u8 *) mmu_to_host(addr);
}

static inline u16 mem_read_halfword(GuestVAddr addr)
{
    return *(u16 *) mmu_to_host(addr);
}

static inline u32 mem_read_word(GuestVAddr addr)
{
    return *(u32 *) mmu_to_host(addr);
}

static inline u64 mem_read_doubleword(GuestVAddr addr)
{
    return *(u64 *) mmu_to_host(addr);
}

#endif // MEMORY_H
