#ifndef MEMORY_H
#define MEMORY_H

#include "mmu.h"
#include "err.h"

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

ResultVoid mem_load_elf(Memory *mem, FILE *f);
ResultVoid mem_load_bin(Memory *mem, FILE *f, GuestVAddr base);
Result(GuestVAddr) mem_alloc(Memory *mem, i64 size);
void mem_clear(Memory *mem);

// static __ForceInline void mem_write(GuestVAddr addr, const void *data, u64 len)
// {
//     memcpy((void *) mmu_to_host(addr), data, len);
// }
//
// static __ForceInline __Keep void mem_read(GuestVAddr addr, void *buf, u64 len)
// {
//     memcpy(buf, (void *) mmu_to_host(addr), len);
// }
//
// #define DECL_MEM_WRITE_X(type) \
// static __ForceInline void mem_write_##type(GuestVAddr addr, type val) \
// { \
//     *(type *) mmu_to_host(addr) = val; \
// }
//
// DECL_MEM_WRITE_X(u8)
// DECL_MEM_WRITE_X(u16)
// DECL_MEM_WRITE_X(u32)
// DECL_MEM_WRITE_X(u64)
//
// #define DECL_MEM_READ_X(type) \
// static __ForceInline type mem_read_##type(GuestVAddr addr) \
// { \
//     return *(type *) mmu_to_host(addr); \
// }
//
// DECL_MEM_READ_X(u8)
// DECL_MEM_READ_X(u16)
// DECL_MEM_READ_X(u32)
// DECL_MEM_READ_X(u64)
// DECL_MEM_READ_X(i8)
// DECL_MEM_READ_X(i16)
// DECL_MEM_READ_X(i32)
// DECL_MEM_READ_X(i64)


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
