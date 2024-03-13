#ifndef ELF_H
#define ELF_H

#include "types.h"

#define ELF_MAGIC_NUMBER 0x7F

#define ET_NONE 0x00
#define ET_EXEC 0x02

typedef struct elf_header {
	u8 magic_number;
	u8 elf_ascii[3];
	u8 class;
	u8 data;
	u8 version;
	u8 os_abi;
	u8 padding[8];
	u16 type;
	u16 machine;
	u32 elf_version;
	u32 entry;
	u32 phoff;
	u32 shoff;
	u32 flags;
	u16 header_size;
	u16 ph_size;
	u16 ph_num;
	u16 sh_size;
	u16 sh_num;
	u16 strtab_idx;
} __attribute__((packed)) elf_header_t;

#define PT_NULL 0x0
#define PT_LOAD 0x1

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x3

typedef struct elf_program_header {
	u32 type;
	u32 offset;
	u32 vaddr;
	u32 paddr;
	u32 filesz;
	u32 memsz;
	u32 flags;
	u32 align;
} __attribute__((packed)) elf_program_header_t;

typedef struct elf_section_header {
	u32 name;
	u32 type;
	u32 flags;
	u32 addr;
	u32 offset;
	u32 size;
	u32 link;
	u32 info;
	u32 addralign;
	u32 entsize;
} __attribute__((packed)) elf_section_header_t;

elf_header_t *elf_load(u32 *data);
i32 is_elf(elf_header_t *elf);
void load_segment(u32 *data, elf_program_header_t *program_header);
void elf_unload(elf_header_t *elf);

#endif
