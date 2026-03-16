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

#define PT_LOAD         1       /* Loadable program segment */

typedef struct {
    u8  ident[EI_NIDENT]; /* Magic number and other info */
    u16 type;             /* Object file type */
    u16 machine;          /* Architecture */
    u32 version;          /* Object file version */
    u64 entry;            /* Entry point virtual address */
    u64 phoff;            /* Program header table file offset */
    u64 shoff;            /* Section header table file offset */
    u32 flags;            /* Processor-specific flags */
    u16 ehsize;           /* ELF header size in bytes */
    u16 phentsize;        /* Program header table entry size */
    u16 phnum;            /* Program header table entry count */
    u16 shentsize;        /* Section header table entry size */
    u16 shnum;            /* Section header table entry count */
    u16 shstrndx;         /* Section header string table index */
} ELFHeader;

#endif // ELF64_H
