#ifndef MEMORY_H
#define MEMORY_H

#include "mmu.h"

#include <string.h>

// host_end = TO_HOST(heap_end)
// heap_end = TO_GUEST(host_end)
typedef struct {
    GuestVAddr entry;       // ELF entry point
    HostVAddr  host_begin;  // Host mmaped region begin address
    HostVAddr  host_end;    // Host mapped region end address
    GuestVAddr heap_base;   // Guest heap base address
    GuestVAddr heap_brk;    // Guest heap program break
    GuestVAddr heap_end;    // Guest heap end address
} Memory;

Memory *mem_create(void);
void mem_destroy(Memory *mem);
void mem_load_elf(Memory *mem, FILE *f);
void mem_load_bin(Memory *mem, FILE *f, GuestVAddr base);
GuestVAddr mem_alloc(Memory *mem, i64 size);

#define mem_write(addr, data, len)  memcpy((void*)mmu_to_host(addr), data, len)
#define mem_read(addr, buf, len)    memcpy(buf, (void*)mmu_to_host(addr), len)

#define mem_write_u8(addr, val)     (*(u8*)mmu_to_host(addr) = val)
#define mem_write_u16(addr, val)    (*(u16*)mmu_to_host(addr) = val)
#define mem_write_u32(addr, val)    (*(u32*)mmu_to_host(addr) = val)
#define mem_write_u64(addr, val)    (*(u64*)mmu_to_host(addr) = val)

#define mem_read_u8(addr)           (*(u8*)mmu_to_host(addr))
#define mem_read_u16(addr)          (*(u16*)mmu_to_host(addr))
#define mem_read_u32(addr)          (*(u32*)mmu_to_host(addr))
#define mem_read_u64(addr)          (*(u64*)mmu_to_host(addr))
#define mem_read_i8(addr)           (*(i8*)mmu_to_host(addr))
#define mem_read_i16(addr)          (*(i16*)mmu_to_host(addr))
#define mem_read_i32(addr)          (*(i32*)mmu_to_host(addr))
#define mem_read_i64(addr)          (*(i64*)mmu_to_host(addr))

#endif // MEMORY_H
