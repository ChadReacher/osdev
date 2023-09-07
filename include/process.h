#ifndef PROCESS_H
#define PROCESS_H

#include <types.h>
#include <paging.h>
#include <isr.h>
#include <vfs.h>

#define ALIGN_DOWN(val, a) ((val) & ~((a) - 1))

typedef enum {
	RUNNABLE,
	RUNNING,
	DEAD
} state_t;

typedef struct {
	u32 edi;
	u32 esi;
	u32 ebx;
	u32 ebp;
	u32 eip;
} context_t;

typedef struct _process {
	u32 pid;
	state_t state;
	struct _process *next;
	struct _process *parent;
	page_directory_t *directory;
	registers_state *regs;
	context_t *context;
	void *kernel_stack_bottom;
	void *kernel_stack_top;
	file *fds;
	i8 *cwd;
} process_t;

void userinit();
process_t *proc_alloc();

#endif
