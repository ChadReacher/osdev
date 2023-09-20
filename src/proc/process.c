#include <process.h>
#include <heap.h>
#include <isr.h>
#include <pmm.h>
#include <tss.h>
#include <stdio.h>
#include <timer.h>
#include <debug.h>
#include <panic.h>
#include <scheduler.h>
#include <string.h>
#include <elf.h>

extern void irq_ret();
extern process_t *proc_list;
extern process_t *current_process;
extern process_t *init_process;
extern process_t *idle_process;

u32 next_pid = 0;

void cpu_idle() {
    while (1) {
		__asm__ __volatile__ ("cli");
        kprintf("kernel idle - %d\n", idle_process->pid);
		__asm__ __volatile__ ("sti");
		__asm__ __volatile__ ("hlt");
	}
}

void userinit() {
	process_t *idle = proc_alloc(); 
    remove_process_from_list(idle);
	idle->regs->eflags = 0x202;
	idle->state = RUNNING;
	idle->parent = NULL;
	idle->directory = virtual_to_physical((void *)0xFFFFF000);
	idle->regs->cs = 0x8;
	idle->regs->ds = 0x8;
	idle->regs->es = 0x8;
	idle->regs->fs = 0x8;
	idle->regs->gs = 0x8;
	idle->regs->useresp = 0x0;
	idle->regs->ss = 0x0;
    idle->regs->eip = (u32)cpu_idle;
    idle->priority = idle->timeslice = 20;
    idle_process = idle;

	vfs_node_t *vfs_node = vfs_get_node("/bin/init");
	if (!vfs_node) {
		PANIC("Failed to start 'init' process\r\n");
	}
	u32 *data = malloc(vfs_node->length);
	memset((i8 *)data, 0, vfs_node->length);
	vfs_read(vfs_node, 0, vfs_node->length, (i8 *)data);

	process_t *new_proc = proc_alloc();
	new_proc->cwd = strdup("/");
	new_proc->directory = paging_copy_page_dir(false);
	init_process = new_proc;
	init_process->parent = NULL; // or current_process ?
	init_process->priority = init_process->timeslice = 20;
	init_process->state = RUNNING;
	//init_process = current_process = proc_list;
	//current_process->parent = NULL; // or current_process ?
	//current_process->priority = current_process->timeslice = 20;
	//current_process->state = RUNNING;


	current_process = init_process;
	void *kernel_page_dir = virtual_to_physical((void *)0xFFFFF000);
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"(current_process->directory));
	elf_load(data);
	free(data);

	i32 argc = 0;
	i32 envc = 1;
	i8 **argv = (i8 **)malloc((argc + 1) * sizeof(i8 *));
	i8 **envp = (i8 **)malloc((envc + 1) * sizeof(i8 *));
    envp[0] = strdup("PATH=/bin");
	argv[argc] = NULL;
	envp[envc] = NULL;
	
	// Handle user stack:
	memset((void *)0xBFFFF000, 0, 4092);
	i8 *usp = (i8 *)0xBFFFFFFB;
	// push envp strings
	for (i32 i = envc - 1; i >= 0; --i) {
		usp -= strlen(envp[i]) + 1;
		strcpy((i8 *)usp, envp[i]);
		free(envp[i]);
		envp[i] = (i8 *)usp;
	}
	// push argv strings
	for (i32 i = argc - 1; i >= 0; --i) {
		usp -= strlen(argv[i]) + 1;
		strcpy((i8 *)usp, argv[i]);
		free(argv[i]);
		argv[i] = (i8 *)usp;
	}

	// Push envp pointers to envp strings
	usp -= (envc + 1) * 4;
	memcpy((void *)usp, (void *)envp, (envc + 1) * 4);

	// Save env ptr
	u32 env_ptr = (u32)usp;

	// Push argv pointers argv strings
	usp -= (argc + 1) * 4;
	memcpy((void *)usp, (void *)argv, (argc + 1) * 4);

	// Save arg ptr
	u32 arg_ptr = (u32)usp;

	usp -= 4;
	*((u32*)usp) = env_ptr;

	usp -= 4;
	*((u32*)usp) = arg_ptr;

	usp -= 4;
	*((u32*)usp) = argc;

	free(argv);
	free(envp);
	init_process->regs->useresp = (u32)usp;
	//current_process->regs->useresp = (u32)usp;

	// Get back to the kernel page directory
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"(kernel_page_dir));
	//current_process = idle;
}

void sleep(void *chan) {
	current_process->wait_chan = chan;
	current_process->state = SLEEPING;
	schedule(NULL);
}

void wakeup(void *chan) {
	for (process_t *p = proc_list; p != NULL; p = p->next) {
		if (p->state == SLEEPING && p->wait_chan == chan) {
			p->wait_chan = NULL;
			p->state = RUNNING;
			p->timeslice = p->priority;
		}
	}
}

void wakeup_proc(process_t *proc) {
	if (proc->state != SLEEPING) {
		return;
	}

	for (process_t *p = proc_list; p != NULL; p = p->next) {
		if (p == proc) {
			p->wait_chan = NULL;
			p->state = RUNNING;
			p->timeslice = p->priority;
		}
	}
}

process_t *proc_alloc() {
	process_t *process = malloc(sizeof(process_t));
	memset((void *)process, 0, sizeof(process_t));

	process->pid = next_pid++;
	process->state = RUNNING;
	process->next = NULL;
	process->fds = malloc(FDS_NUM * sizeof(file));
	memset(process->fds, 0, FDS_NUM * sizeof(file));
	process->kernel_stack_bottom = malloc(4096 * 2);
	memset(process->kernel_stack_bottom, 0, 4096 * 2);
	u32 *sp = (u32 *)ALIGN_DOWN((u32)process->kernel_stack_bottom + 4096 * 2 - 1, 4);

	// Setup kernel stack as we have returned from interrupt routine
	*sp-- = 0x23;			// user DS
	*sp-- = 0xBFFFFFFB;		// user stack
	*sp-- = 0x200;			// EFLAGS
	*sp-- = 0x1B;			// user CS
	*sp-- = 0x0;			// user eip
	*sp-- = 0x0;			// err code
	*sp-- = 0x0;			// int num
	*sp-- = 0x0;			// eax
	*sp-- = 0x0; 			// ecx
	*sp-- = 0x0; 			// edx
	*sp-- = 0x0; 			// ebx
	*sp-- = 0x0; 			// esp
	*sp-- = 0x0;			// ebp
	*sp-- = 0x0; 			// esi
	*sp-- = 0x0; 			// edi
	*sp-- = 0x23;			// ds
	*sp-- = 0x23; 			// es
	*sp-- = 0x23; 			// fs
	*sp-- = 0x23; 			// gs
	process->regs = (registers_state *)(sp + 1);
	*sp-- = (u32)irq_ret;	// irq_ret eip (to return back to the end of the interrupt routine)
	*sp-- = 0x0;			// ebp
	*sp-- = 0x0; 			// ebx
	*sp-- = 0x0; 			// esi
	*sp-- = 0x0; 			// edi
	++sp;

	process->kernel_stack_top = (void *)sp;
	process->context = (context_t *)sp;

	add_process_to_list(process);

	return process;
}

i32 proc_get_fd(process_t *proc) {
	for (u32 i = 3; i < FDS_NUM; ++i) {
		if (!proc->fds[i].used) {
			proc->fds[i].used = true;
			return i;
		}
	}
	return -1;
}
