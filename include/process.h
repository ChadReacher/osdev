#ifndef PROCESS_H
#define PROCESS_H

#include <types.h>
#include <isr.h>

typedef struct _process {
	struct _process *next;
	struct _process *prev;
	u32 pid;
	u32 esp;
	u32 eip;
	void *directory;
	registers_state regs;
} process_t;

void process_init();
void process_handler(registers_state regs);
void proc_run_code(u8 *code, i32 len);
void switch_process(process_t * process);
void schedule();

#endif
