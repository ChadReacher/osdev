#include <process.h>
#include <list.h>
#include <heap.h>
#include <isr.h>
#include <pmm.h>
#include <tss.h>
#include <paging.h>
#include <stdio.h>
#include <timer.h>

extern page_directory_t *cur_page_dir;

extern u32 task;

process_t *current_process;

u32 pid_c = 0;

void kidle() {
	while (1);
}

void task1() {
	while (1) kprintf("1");
}

void task2() {
	while (1) kprintf("2");
}

process_t *create_process(u32 addr) {
	process_t *p = malloc(sizeof(process_t));
	memset(p, 0, sizeof(process_t));
	p->pid = pid_c++;
	p->eip = addr;
	p->esp = (u32)malloc(4096);
	u32 *stack = p->esp + 4096;
	*--stack = 0x00000202;
	*--stack = 0x8;
	*--stack = addr;
	*--stack = 0;
	*--stack = 0;
	*--stack = 0;
	*--stack = 0;
	*--stack = 0;
	*--stack = 0;
	*--stack = p->esp + 4096;
	*--stack = 0x10;
	*--stack = 0x10;
	*--stack = 0x10;
	*--stack = 0x10;
	p->esp = (u32)stack;
	return p;
}

void add_process(process_t *p) {
	//__asm __volatile__ ("cli");
	task = 0;
	p->next = current_process->next;
	p->next->prev = p;
	p->prev = current_process;
	current_process->next = p;
	task = 1;
	//__asm __volatile__ ("sti");
}

void schedule() {
	__asm__ __volatile__ ("push %eax");
	__asm__ __volatile__ ("push %ebx");
	__asm__ __volatile__ ("push %ecx");
	__asm__ __volatile__ ("push %edx");
	__asm__ __volatile__ ("push %esi");
	__asm__ __volatile__ ("push %edi");
	__asm__ __volatile__ ("push %ebp");
	__asm__ __volatile__ ("push %ds");
	__asm__ __volatile__ ("push %es");
	__asm__ __volatile__ ("push %fs");
	__asm__ __volatile__ ("push %gs");
	__asm__ __volatile__ ("mov %%esp, %%eax" : "=a"(current_process->esp));
	current_process = current_process->next;
	__asm__ __volatile__ ("mov %%eax, %%esp" : : "a"(current_process->esp));
	__asm__ __volatile__ ("pop %gs");
	__asm__ __volatile__ ("pop %fs");
	__asm__ __volatile__ ("pop %es");
	__asm__ __volatile__ ("pop %ds");
	__asm__ __volatile__ ("pop %ebp");
	__asm__ __volatile__ ("pop %edi");
	__asm__ __volatile__ ("pop %esi");
	__asm__ __volatile__ ("pop %edx");
	__asm__ __volatile__ ("pop %ecx");
	__asm__ __volatile__ ("pop %ebx");
	__asm__ __volatile__ ("pop %eax");
	__asm__ __volatile__ ("iret");
}


void process_init() {
	current_process = create_process(kidle);
	current_process->next = current_process;
	current_process->prev = current_process;
	add_process(create_process((u32)task1));
	add_process(create_process((u32)task2));
	kprintf("Tasking online!\n");
	//process_t *kernel_proc = malloc(sizeof(process_t));

	//kernel_proc->next = kernel_proc;
	//kernel_proc->pid = 0;

	//current_process = NULL;

	//register_interrupt_handler(IRQ0, process_handler);
}

void process_handler(registers_state regs) {
	if (!current_process || current_process->next == current_process) {
		return;
	}

	current_process->regs = regs;

	if (current_process->next) {
		switch_process(current_process->next);
	}
}

void proc_run_code(u8 *code, i32 len) {
	process_t *process = malloc(sizeof(process_t));
	void *page_directory_phys = allocate_blocks(1);
	void *code_phys = allocate_blocks(2);
	void *stack_phys = allocate_blocks(2);

	// Create a new address space for a process
	// 1. Copy current page directory
	map_page(page_directory_phys, 0xB0000000);
	u32 *arr = (u32 *)0xB0000000;
	u32 *arr2 = (u32 *)0xFFFFF000;
	for (u32 i = 0; i < 1024; ++i) {
		arr[i] = arr2[i];
	}
	//memcpy((void *)0xB0000000, (void *)0xFFFFF000, 4096);
	page_directory_entry *pd = (page_directory_entry *)0xB0000000;
	pd[0] = (u32)code_phys | PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE; 
	pd[767] = (u32)stack_phys | PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE; 
	unmap_page(0xB0000000);


	map_page(code_phys, 0xB0000000);
	memcpy((void *)0xB0000000, code_phys, len);
	unmap_page(0xB0000000);

	process->pid = 33;
	process->directory = page_directory_phys;

	if (current_process && current_process->next) {
		process_t *current_last = current_process->next;

		while (current_last->next != current_process) {
			current_last = current_last->next;
		}
		current_last->next = process;
		process->next = current_process;
	}

	switch_process(process);
}

void switch_process(process_t *process) {
	u32 esp0 = 0;
	__asm__ __volatile__ ("mov %%esp, %0" : "=r"(esp0));
	tss_set_stack(0x10, esp0);

	cur_page_dir = (page_directory_t *)(process->directory);
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"(cur_page_dir));

	__asm__ __volatile__ (
			"push $0x23\n"
			"push $0xBFFFFFFB\n"
			"push $512\n"
			"push $0x1B\n"
			"push $0x00000000\n"
			"iret\n"
	);
}


