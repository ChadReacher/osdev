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

extern page_directory_t *cur_page_dir;

process_t *current_process;

void process_init() {
	switch_process(NULL);
}

void process_create(u8 *code, i32 len) {
	process_t *process = malloc(sizeof(process_t));
	memset(process, 0, sizeof(process_t));

	void *code_phys_frame = allocate_blocks(1);
	void *stack_phys_frame = allocate_blocks(1);

	// 1. Create a new address space for a process
	// (Duplicate the current page directory)
	void *new_page_dir_phys = (page_directory_t *)allocate_blocks(1);
	map_page(new_page_dir_phys, 0xE0000000); // Temporary mapping

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
	map_page(code_phys_frame, (void *)0x0);
	memcpy((void *)0x0, code, len);
	

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
	map_page(stack_phys_frame, (void *)0xBFFFFFFB);

	page_directory_entry *stack_pd_entry = &cur_pd->entries[767];
	*stack_pd_entry |= PAGING_FLAG_USER;

	page_table_t *stack_page_table = (page_table_t *)(0xFFC00000 + (PAGE_DIR_INDEX((u32)0xBFFFFFFB) << 12));

	page_table_entry page_for_stack = 0;
	page_for_stack |= PAGING_FLAG_PRESENT;
	page_for_stack |= PAGING_FLAG_WRITEABLE;
	page_for_stack |= PAGING_FLAG_USER;
	page_for_stack = ((page_for_stack & ~0xFFFFF000) | (physical_address)stack_phys_frame);
	stack_page_table->entries[PAGE_TABLE_INDEX(0xBFFFFFFB)] = page_for_stack;
	
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"(previous_page_dir));

	process->next = process;
	process->directory = new_page_dir_phys;
	process->regs.eip = 0;
	process->regs.cs = 0x1B;
	process->regs.ds = 0x23;
	process->regs.useresp = 0xBFFFFFFB;

	if (current_process && current_process->next) {
		process_t *p = current_process->next;
		current_process->next = process;
		process->next = p;
	} else if (!current_process) {
		current_process = process;
		current_process->next = current_process;
	}
}

void switch_process(registers_state *regs) {
	if (regs) {
		current_process->regs = *regs;
	}

	current_process = current_process->next;
	if (!current_process) {
		PANIC("ERROR");
	}

	u32 esp0;
	__asm__ __volatile__ ("mov %%esp, %0\n" : "=r"(esp0));
	tss_set_stack(esp0);

	cur_page_dir = (page_directory_t *)(current_process->directory);
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"((u32)current_process->directory));

	__asm__ __volatile__ (
			"push $0x23\n"
			"push $0xBFFFFFFB\n"
			"push $512\n"
			"push $0x1B\n"
			"push $0x00000000\n"
			"iret\n"
	);
}

