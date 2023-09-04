#include <scheduler.h>
#include <tss.h>
#include <panic.h>
#include <vfs.h>
#include <elf.h>

extern void context_switch(u32 eax, u32 ecx, u32 edx, u32 ebx, u32 useresp, u32 ebp, u32 esi, u32 edi, u32 eip);

process_t *proc_list = NULL;
process_t *current_process = NULL;

void run_init_process(i8 *file) {
	__asm__ __volatile__ ("cli");

	vfs_node_t *vfs_node = vfs_get_node(file);
	if (!vfs_node) {
		PANIC("Failed to start 'init' process\r\n");
	}
	u32 *data = malloc(vfs_node->length);
	memset((i8 *)data, 0, vfs_node->length);
	vfs_read(vfs_node, 0, vfs_node->length, (i8 *)data);
	elf_load(data);

	current_process = proc_list;
	current_process->state = RUNNING;

	tss_set_stack(current_process->kernel_stack_top);

	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"((u32)current_process->directory));

	u32 proc_eax = current_process->regs.eax;
	u32 proc_ecx = current_process->regs.ecx;
	u32 proc_edx = current_process->regs.edx;
	u32 proc_ebx = current_process->regs.ebx;
	u32 proc_esp = current_process->regs.useresp;
	u32 proc_ebp = current_process->regs.ebp;
	u32 proc_esi = current_process->regs.esi;
	u32 proc_edi = current_process->regs.edi;
	u32 proc_eip = current_process->regs.eip;

	context_switch(proc_eax, proc_ecx, proc_edx, proc_ebx, proc_esp, proc_ebp, proc_esi, proc_edi, proc_eip);
}

void scheduler_init() {
	register_interrupt_handler(IRQ0, schedule);
}

process_t *get_current_process() {
	return current_process;
}

process_t *get_next_process() {
	if (!current_process) {
		return NULL;
	}
	process_t *next = current_process->next;
	if (!next) {
		next = proc_list;
	}
	return next;
}

void add_process_to_list(process_t *new_proc) {
	process_t *tmp = proc_list;
	new_proc->next = NULL;
	
	if (!tmp) {
		proc_list = new_proc;
		return;
	}

	while (tmp->next) {
		tmp = tmp->next;
	}
	tmp->next = new_proc;
}

void remove_process_from_list(process_t *proc) {
	process_t *tmp = proc_list;
	if (!tmp) {
		return;
	}

	process_t *prev = NULL;
	while (tmp) {
		if (tmp == proc) {
			if (prev) {
				prev->next = tmp->next;
			} else {
				proc_list = NULL;
			}
			return;
		}
		prev = tmp;
		tmp = tmp->next;
	}
}


// New page directory, user stack and user code, kernel stack
// 1. Save current registers state to the current process
// 2. Get next process and mark it as current
// 3. Change virtual address space
// 4. Restore current process' registers
// 5. Start the process
void schedule(registers_state *regs) {
	// There are no processes at all
	if (proc_list == NULL) {
		return;
	}

	__asm__ __volatile__ ("cli");
	if (regs) {
		current_process->regs = *regs;
	}
	current_process->state = RUNNABLE;

	process_t *next_proc;
	while (1) {
		// Find the next process to run
		if (current_process->next == NULL) {
			// Are we the last process in the 'proc_list'?
			next_proc = proc_list;
		} else {
			next_proc = current_process->next;
		}

		if (next_proc != NULL && next_proc->state == DEAD) {
			remove_process_from_list(next_proc);
			process_free(next_proc);
		} else {
			next_proc->state = RUNNING;
			break;
		}
	}

	if (next_proc == current_process) {
		return;
	}

	current_process = next_proc;

	tss_set_stack(current_process->kernel_stack_top);

	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"((u32)current_process->directory));

	u32 proc_eax = current_process->regs.eax;
	u32 proc_ecx = current_process->regs.ecx;
	u32 proc_edx = current_process->regs.edx;
	u32 proc_ebx = current_process->regs.ebx;
	u32 proc_esp = current_process->regs.useresp;
	u32 proc_ebp = current_process->regs.ebp;
	u32 proc_esi = current_process->regs.esi;
	u32 proc_edi = current_process->regs.edi;
	u32 proc_eip = current_process->regs.eip;

	context_switch(proc_eax, proc_ecx, proc_edx, proc_ebx, proc_esp, proc_ebp, proc_esi, proc_edi, proc_eip);
}

void process_kill(process_t *proc) {
	__asm__ __volatile__ ("cli");

	if (proc->pid == 1) {
		PANIC("Cannot exit the 'init' process");
	}

	proc->state = DEAD;

	process_t *next_proc;
	while (1) {
		// Find the next process to run
		if (current_process->next == NULL) {
			// Are we the last process in the 'proc_list'?
			next_proc = proc_list;
		} else {
			next_proc = current_process->next;
		}

		if (next_proc != NULL && next_proc->state == DEAD) {
			remove_process_from_list(next_proc);
			process_free(next_proc);
		} else {
			next_proc->state = RUNNING;
			break;
		}
	}

	if (next_proc == current_process) {
		return;
	}

	current_process = next_proc;

	tss_set_stack(current_process->kernel_stack_top);

	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"((u32)current_process->directory));

	u32 proc_eax = current_process->regs.eax;
	u32 proc_ecx = current_process->regs.ecx;
	u32 proc_edx = current_process->regs.edx;
	u32 proc_ebx = current_process->regs.ebx;
	u32 proc_esp = current_process->regs.useresp;
	u32 proc_ebp = current_process->regs.ebp;
	u32 proc_esi = current_process->regs.esi;
	u32 proc_edi = current_process->regs.edi;
	u32 proc_eip = current_process->regs.eip;

	context_switch(proc_eax, proc_ecx, proc_edx, proc_ebx, proc_esp, proc_ebp, proc_esi, proc_edi, proc_eip);
}
