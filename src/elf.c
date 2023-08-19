#include "elf.h"
#include "string.h"
#include "debug.h"
#include "paging.h"

i32 is_elf(elf_header_t *elf) {
	i32 ret = -1;
	if (elf->magic_number == ELF_MAGIC_NUMBER && strncmp((i8 *)elf->elf_ascii, "ELF", 3) == 0) {
		ret = 0;
	}

	if (ret != -1) {
		ret = elf->type;
	}

	return ret;
}

elf_header_t *elf_load(u32 *data) {
	elf_header_t *elf = (elf_header_t *)data;

	if (is_elf(elf) != ET_EXEC) {
		DEBUG("%s", "This elf file is not an executable\r\n");
		return NULL;
	}

	DEBUG("file header: machine = 0x%x, version = 0x%x entry = %p\r\n",
			elf->machine, elf->version, elf->entry);
	DEBUG("header_size = 0x%x, ph_off = 0%x\r\n",
			elf->header_size, elf->phoff);
	DEBUG("ph_size = 0x%x, ph_num = 0x%x\r\n", elf->ph_size, elf->ph_num);
	DEBUG("sh_size = 0x%x, sh_num = 0x%x\r\n", elf->sh_size, elf->sh_num);

	elf_program_header_t* program_header = (elf_program_header_t*)((u32)data + elf->phoff);

	elf->entry += 0x100000;

	for (u32 i = 0; i < elf->ph_num; ++i) {
		DEBUG("Program header: type - %d, vaddr = %p\r\n",
				program_header[i].type,
				program_header[i].vaddr
		);

		if (program_header[i].type == PT_LOAD) {
			load_segment(data, &program_header[i]);
		}
	}

	return elf;
}

void load_segment(u32 *data, elf_program_header_t *program_header) {
	u32 memsz = program_header->memsz; // Size in memory
	u32 filesz = program_header->filesz; // Size in file
	u32 vaddr = program_header->vaddr; // Offset in memory
	u32 offset = program_header->offset; // Offset in file

	u32 paging_flags = PAGING_FLAG_PRESENT;

	if ((program_header->flags & PF_W) == PF_W) {
		paging_flags |= PAGING_FLAG_WRITEABLE;
	}

	vaddr += 0x100000;

	map_page(virtual_to_physical((void *)vaddr), (void *)vaddr);	
	
	if (memsz == 0) {
		return;
	}

	memcpy((void *)vaddr, &data[offset], filesz);
	memset((void *)(vaddr + filesz), 0, memsz - filesz);
}
