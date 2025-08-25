#include <process.h>
#include <heap.h>
#include <isr.h>
#include <pmm.h>
#include <tss.h>
#include <stdio.h>
#include <timer.h>
#include <panic.h>
#include <scheduler.h>
#include <string.h>
#include <elf.h>
#include <vfs.h>

extern struct proc *procs[NR_PROCS];
extern void enter_usermode_asm(u32 useresp);
extern int need_resched;

struct file file_table[NR_FILE];
u32 next_pid = 0;

void process_sleep(struct proc **p ) {
	if (!p) {
		return;
	}
	*p = current_process;
	current_process->state = INTERRUPTIBLE;
	schedule();
}

void process_wakeup(struct proc **p) {
	if (p && *p) {
		(**p).state = RUNNING;
		*p = NULL;
		need_resched = 1;
	}
}

void enter_usermode(void) {
	current_process = procs[1];
	
	tss_set_stack((u32)current_process->kernel_stack_top);
	__asm__ volatile ("movl %%eax, %%cr3" 
            : 
            : "a"((u32)current_process->directory));

	debug("Entering user space with INIT process\r\n");
	enter_usermode_asm(current_process->regs->useresp);
}

i32 syscall_exec(i8 *pathname, i8 **u_argv, i8 **u_envp);

#define INIT_PID 1
#define INIT_PROGRAM "/bin/init"

void user_init(void) {
    i8 *argv[] = { INIT_PROGRAM, NULL };
    i8 *envp[] = { "PATH=/bin", NULL };
	struct proc *init_process = procs[INIT_PID];

	init_process->directory = paging_copy_page_dir(0);
	physical_address curr_page_dir_phys = virtual_to_physical(CURR_PAGE_DIR);

	__asm__ volatile ("movl %%eax, %%cr3" : : "a"(init_process->directory));

    i32 err = syscall_exec(INIT_PROGRAM, argv, envp);
    if (err != 0) {
        panic("Failed to load the 'INIT' program\r\n"); 
    }

	__asm__ volatile ("movl %%eax, %%cr3" : : "a"(curr_page_dir_phys));
}

i32 get_new_fd() {
	i32 fd;
	for (fd = 0; fd < NR_OPEN; ++fd) {
		if (!current_process->fds[fd]) {
			break;
		}
	}
	return fd;
}

struct file *get_empty_file() {
	i32 i;
	for (i = 0; i < NR_FILE; ++i) {
		if (!file_table[i].f_count) {
			return &file_table[i];
		}
	}
	return NULL;
}

