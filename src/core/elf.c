#include <elf.h>
#include <string.h>
#include <debug.h>
#include <paging.h>
#include <process.h>

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

	if (memsz == 0) {
		return;
	}

	void *code = (void *)((u32)data + offset);
	process_t *new_proc = proc_alloc();
	new_proc->cwd = strdup("/");
	new_proc->directory = paging_copy_page_dir(false);

	// Move to the newly created address space and
	// setup pages for user code and stack
	u32 len_in_blocks = filesz / 4096;
	if (len_in_blocks % 4096 != 0 || len_in_blocks == 0) {
		++len_in_blocks;
	}
	void *code_phys_frame = allocate_blocks(len_in_blocks);
	void *stack_phys_frame = allocate_blocks(1);

	void *kernel_page_dir = virtual_to_physical(0xFFFFF000);
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"(new_proc->directory));
	page_directory_t *cur_pd = (page_directory_t *)0xFFFFF000;

	// Create mapping for user code
	for (u32 i = 0, addr = 0x0; i < len_in_blocks; ++i, addr += 0x1000) {
		u32 code_page = (u32)code_phys_frame + addr;
		map_page((void *)code_page, (void *)addr);
		memcpy((void *)addr, code + addr, 0x1000);
	}

	page_directory_entry *code_pd_entry = &cur_pd->entries[0];
	*code_pd_entry |= PAGING_FLAG_USER;

	page_table_t *code_page_table = (page_table_t *)(0xFFC00000 + (PAGE_DIR_INDEX((u32)0x0) << 12));

	page_table_entry page_for_code = 0;
	page_for_code |= PAGING_FLAG_PRESENT;
	page_for_code |= PAGING_FLAG_WRITEABLE;
	page_for_code |= PAGING_FLAG_USER;
	page_for_code = ((page_for_code & ~0xFFFFF000) | (physical_address)code_phys_frame);
	code_page_table->entries[0] = page_for_code;

	// Create mapping for user stack
	map_page(stack_phys_frame, (void *)0xBFFFF000);

	page_directory_entry *stack_pd_entry = &cur_pd->entries[767];
	*stack_pd_entry |= PAGING_FLAG_USER;

	page_table_t *stack_page_table = (page_table_t *)(0xFFC00000 + (PAGE_DIR_INDEX((u32)0xBFFFF000) << 12));

	page_table_entry page_for_stack = 0;
	page_for_stack |= PAGING_FLAG_PRESENT;
	page_for_stack |= PAGING_FLAG_WRITEABLE;
	page_for_stack |= PAGING_FLAG_USER;
	page_for_stack = ((page_for_stack & ~0xFFFFF000) | (physical_address)stack_phys_frame);
	stack_page_table->entries[PAGE_TABLE_INDEX(0xBFFFF000)] = page_for_stack;
	
	// Get back to the kernel page directory
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"(kernel_page_dir));
}

void elf_unload(elf_header_t *elf) {
	elf_program_header_t* program_header = (elf_program_header_t*)((u32)elf + elf->phoff);
	for (u32 i = 0; i < elf->ph_num; ++i) {
		if (program_header[i].type == PT_LOAD) {
			u32 vaddr = program_header[i].vaddr; // Offset in memory
			void *paddr = virtual_to_physical(vaddr);
			free_blocks(paddr, 1);
			unmap_page((void *)vaddr);
		}
	}
}
