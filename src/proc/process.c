#include <process.h>
#include <list.h>
#include <heap.h>
#include <isr.h>
#include <pmm.h>
#include <tss.h>
#include <paging.h>
#include <stdio.h>
#include <timer.h>
#include <debug.h>
#include <panic.h>
#include <scheduler.h>
#include <paging.h>

u32 next_pid = 1;

void process_create(u8 *code, i32 len) {
	process_t *process = malloc(sizeof(process_t));
	memset(process, 0, sizeof(process_t));

	u32 len_in_blocks = len / 4096;
	if (len_in_blocks % 4096 != 0 || len_in_blocks == 0) {
		++len_in_blocks;
	}
	void *code_phys_frame = allocate_blocks(len_in_blocks);
	void *stack_phys_frame = allocate_blocks(1);

	// 1. Create a new address space for a process
	// (Duplicate the current page directory)

	// Link kernel pages and set up recursive paging
	void *new_page_dir_phys = (page_directory_t *)allocate_blocks(1);
	map_page(new_page_dir_phys, 0xE0000000);
	memset(0xE0000000, 0, 4096);
	page_directory_t *pd = (page_directory_t *)0xE0000000;
	page_directory_t *cur_pd = (page_directory_t *)0xFFFFF000;
	for (u32 i = 768; i < 1024; ++i) {
		pd->entries[i] = cur_pd->entries[i];
	}
	pd->entries[1023] = (u32)new_page_dir_phys | PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE;
	for (u32 i = 0; i < (0xC0000000 >> 22); ++i) {
		pd->entries[i] = 0;
	}
	unmap_page(0xE0000000);

	void *previous_page_dir = virtual_to_physical(0xFFFFF000);
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"(new_page_dir_phys));

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
	
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"(previous_page_dir));

	process->directory = new_page_dir_phys;
	void *kernel_stack = malloc(4096);
	process->kernel_stack_bottom = kernel_stack;
	process->kernel_stack_top = (u8 *)kernel_stack + 4096 - 1;
	process->regs.eip = 0;
	process->regs.cs = 0x1B;
	process->regs.ds = 0x23;
	process->regs.useresp = 0xBFFFFFFB;
	process->regs.ebp = 0xBFFFFFFB;
	process->pid = next_pid++;
	process->state = RUNNABLE;

	add_process_to_list(process);
}

void process_free(process_t *proc) {
	free(proc->kernel_stack_bottom);

	void *page_dir_phys = (void *)proc->directory;
	map_page(page_dir_phys, 0xE0000000);
	page_directory_t *page_dir = (page_directory_t *)0xE0000000;
	for (u32 i = 0; i < 768; ++i) {
		if (!page_dir->entries[i]) {
			continue;
		}
		page_directory_entry pde = page_dir->entries[i];
		void *table_phys = (void *)GET_FRAME(pde);
		map_page(table_phys, 0xEA000000); // Temporary mapping
		page_table_t *table = (page_table_t *)0xEA000000;
		for (u32 j = 0; j < 1024; ++j) {
			if (!table->entries[j]) {
				continue;
			}
			page_table_entry pte = table->entries[j];
			void *page_frame = (void *)GET_FRAME(pte);
			free_blocks(page_frame, 1);
		}
		unmap_page(0xEA000000);
		free_blocks(table_phys, 1);
	}
	unmap_page(0xE0000000);

	free_blocks(page_dir_phys, 1);
	free(proc);
}
