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

	/*__asm__ __volatile__ ("sti");*/
	enter_usermode_asm(current_process->regs->useresp);
}

void user_init() {
	struct ext2_inode *inode;
	i32 err, i;
	i8 **argv, **envp;
	void *kernel_page_dir;
	i32 argc = 0;
	i32 envc = 1;

	err = namei("/bin/init", &inode);
	if (err) {
		panic("could not find '/bin/init' executable\n");
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

	for (i = 0; i < NR_GROUPS; ++i) {
		init_process->groups[i] = -1;
	}
	init_process->sleep = 0;
	init_process->tty = -1;
	init_process->directory = paging_copy_page_dir(0);
	kernel_page_dir = virtual_to_physical((void *)0xFFFFF000);
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"(init_process->directory));
	if ((err = elf_load(inode, argc, argv, envc, envp))) {
		iput(inode);
		panic("could not load '/bin/init' executable\n");
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

