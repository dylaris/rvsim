#ifndef ELF64_H
#define ELF64_H

#define EI_NIDENT       16

#define ELFMAG          "\177ELF"

#define EM_RISCV        243     /* RISC-V */

#define EI_CLASS        4       /* File class byte index */
#define ELFCLASSNONE    0       /* Invalid class */
#define ELFCLASS32      1       /* 32-bit objects */
#define ELFCLASS64      2       /* 64-bit objects */
#define ELFCLASSNUM     3

#define PT_LOAD 1         /* Loadable program segment */

#define PF_X    (1 << 0)  /* Segment is executable */
#define PF_W    (1 << 1)  /* Segment is writable */
#define PF_R    (1 << 2)  /* Segment is readable */

#define R_X86_64_PC32 2

typedef struct {
    u8  ident[EI_NIDENT];
    u16 type;
    u16 machine;
    u32 version;
    u64 entry;
    u64 phoff;
    u64 shoff;
    u32 flags;
    u16 ehsize;
    u16 phentsize;
    u16 phnum;
    u16 shentsize;
    u16 shnum;
    u16 shstrndx;
} ELFHeader;

typedef struct {
    u32 type;
    u32 flags;
    u64 offset;
    u64 vaddr;
    u64 paddr;
    u64 filesz;
    u64 memsz;
    u64 align;
} ProgHeader;

typedef struct {
    u32 name;
    u32 type;
    u32 flags;
    u64 addr;
    u64 offset;
    u64 size;
    u32 link;
    u32 info;
    u64 addralign;
    u64 entsize;
} SecHeader;

typedef struct {
	u32 name;
	u8  info;
	u8  other;
	u16 shndx;
	u64 value;
	u64 size;
} Sym;

typedef struct {
    u64 offset;
    u32 type;
    u32 sym;
    i64 addend;
} Rela;

#endif // ELF64_H
