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
#include <queue.h>
#include <signal.h>
#include <ext2.h>

extern void irq_ret();
extern queue_t *ready_queue;
extern queue_t *procs;
extern process_t *current_process;
extern process_t *init_process;
extern process_t *idle_process;
extern void enter_usermode_asm(u32 useresp);

struct file file_table[NR_FILE];
u32 next_pid = 0;

void enter_usermode() {
	current_process = queue_dequeue(ready_queue);
	
	tss_set_stack((u32)current_process->kernel_stack_top);
	__asm__ __volatile__ ("movl %%eax, %%cr3" 
            : 
            : "a"((u32)current_process->directory));

	__asm__ __volatile__ ("sti");
	enter_usermode_asm(current_process->regs->useresp);
}


void userinit() {
	struct ext2_inode *inode;
	i32 err, i;
	i8 **argv, **envp;
	void *kernel_page_dir;
	i32 argc = 0;
	i32 envc = 1;

	inode = namei("/bin/init");
	if (!inode) {
		return;
	} else if (!EXT2_S_ISREG(inode->i_mode)) {
		iput(inode);
		return;
	} else if (!check_permission(inode, MAY_READ | MAY_EXEC)) {
		iput(inode);
		return;
	}
	argc = 0;
	envc = 1;
	argv = (i8 **)malloc((argc + 1) * sizeof(i8 *));
	envp = (i8 **)malloc((envc + 1) * sizeof(i8 *));
	envp[0] = strdup("PATH=/bin");
	argv[argc] = NULL;
	envp[envc] = NULL;
	current_process = init_process;
	init_process->parent = NULL;
	init_process->directory = paging_copy_page_dir(0);
	kernel_page_dir = virtual_to_physical((void *)0xFFFFF000);
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"(init_process->directory));
	if ((err = elf_load(inode, argc, argv, envc, envp))) {
		iput(inode);
		return;
	}
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"(kernel_page_dir));
	for (i = 0; i < NSIG; ++i) {
		sighandler_t hand = current_process->signals[i].sa_handler;
		if (hand != SIG_DFL && hand != SIG_IGN && hand != SIG_ERR) {
			current_process->signals[i].sa_handler = SIG_DFL;
		}
	}
	current_process->close_on_exec = 0;
	iput(inode);
}

process_t *proc_alloc() {
	u32 *sp;
	process_t *process = malloc(sizeof(process_t));
	if (!process) {
		return NULL;
	}
	memset((void *)process, 0, sizeof(process_t));

	process->pid = next_pid++;
	process->timeslice = 20;
	process->state = RUNNING;
	/*process->fds = malloc(FDS_NUM * sizeof(file)); */
	/*if (!process->fds) { */
	/*	return NULL; */
	/*} */
	/*memset(process->fds, 0, FDS_NUM * sizeof(file)); */
	process->kernel_stack_bottom = malloc(4096 * 2);
	if (!process->kernel_stack_bottom) {
		return NULL;
	}
	memset(process->kernel_stack_bottom, 0, 4096 * 2);
	sp = (u32 *)ALIGN_DOWN((u32)process->kernel_stack_bottom + 4096 * 2 - 1, 4);

	/* Setup kernel stack as we have returned from interrupt routine */
	*sp-- = 0x23;			/* user DS */
	*sp-- = 0xBFFFFFFB;		/* user stack */
	*sp-- = 0x200;			/* EFLAGS */
	*sp-- = 0x1B;			/* user CS */
	*sp-- = 0x0;			/* user eip */
	*sp-- = 0x0;			/* err code */
	*sp-- = 0x0;			/* int num */
	*sp-- = 0x0;			/* eax */
	*sp-- = 0x0; 			/* ecx */
	*sp-- = 0x0; 			/* edx */
	*sp-- = 0x0; 			/* ebx */
	*sp-- = 0x0; 			/* esp */
	*sp-- = 0x0;			/* ebp */
	*sp-- = 0x0; 			/* esi */
	*sp-- = 0x0; 			/* edi */
	*sp-- = 0x23;			/* ds */
	*sp-- = 0x23; 			/* es */
	*sp-- = 0x23; 			/* fs */
	*sp-- = 0x23; 			/* gs */
	process->regs = (registers_state *)(sp + 1);
	*sp-- = (u32)irq_ret;	/* irq_ret eip (to return back to the end of the interrupt routine) */
	*sp-- = 0x0;			/* ebp */
	*sp-- = 0x0; 			/* ebx */
	*sp-- = 0x0; 			/* esi */
	*sp-- = 0x0; 			/* edi */
	++sp;

	process->kernel_stack_top = (void *)sp;
	process->context = (context_t *)sp;

	sigemptyset(&process->sigpending);
	sigemptyset(&process->sigmask);

	queue_enqueue(ready_queue, process);
	queue_enqueue(procs, process);
	return process;
}

i32 get_new_fd() {
	i32 fd;
	for (fd = 3; fd < NR_OPEN; ++fd) {
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

