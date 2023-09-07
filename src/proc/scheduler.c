#include <scheduler.h>
#include <tss.h>
#include <panic.h>
#include <vfs.h>
#include <elf.h>
#include <debug.h>

extern void switch_to(context_t **old_context, context_t *new_context);
extern void enter_usermode();

process_t *proc_list = NULL;
process_t *current_process = NULL;

void scheduler_init() {
	register_interrupt_handler(IRQ0, schedule);
	tss_set_stack(current_process->kernel_stack_top);
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"((u32)current_process->directory));

	enter_usermode();
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

void schedule(registers_state *regs) {
	(void)regs;

	if (current_process == proc_list && proc_list->next == NULL) {
		return;
	}
	process_t *old = current_process;
	if (current_process->next == NULL) {
		current_process = proc_list;
	} else {
		current_process = current_process->next;
	}
	tss_set_stack((u32)current_process->kernel_stack_top);
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"((u32)current_process->directory));
	switch_to(&old->context, current_process->context);
}
