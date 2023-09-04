#ifndef PROCESS_H
#define PROCESS_H

#include <types.h>
#include <paging.h>
#include <isr.h>
#include <vfs.h>

typedef enum {
	RUNNABLE,
	RUNNING,
	DEAD
} state_t;

typedef struct _process {
	u32 pid;
	state_t state;
	struct _process *next;
	page_directory_t *directory;
	registers_state regs;
	void *kernel_stack_bottom;
	void *kernel_stack_top;
	file *fds;
	i8 *cwd;
} process_t;

void process_create(u8 *code, i32 len);

#endif
