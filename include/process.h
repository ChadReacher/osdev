#ifndef PROCESS_H
#define PROCESS_H

#include <types.h>
#include <paging.h>
#include <isr.h>

typedef struct _process {
	u32 pid;
	struct _process *next;
	page_directory_t *directory;
	registers_state regs;
	void *kernel_stack;
} process_t;

void process_init();
void create_init_task(i8 *file);
void process_create(u8 *code, i32 len);
void switch_process(registers_state *regs);

#endif
