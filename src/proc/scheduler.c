#include <scheduler.h>
#include <tss.h>
#include <panic.h>
#include <vfs.h>
#include <elf.h>
#include <debug.h>
#include <screen.h>

extern void switch_to(context_t **old_context, context_t *new_context);
extern void enter_usermode(u32 useresp);

process_t *proc_list = NULL;
process_t *current_process = NULL;
process_t *init_process = NULL;

void scheduler_init() {
	register_interrupt_handler(IRQ0, schedule);
	//tss_set_stack((u32)current_process->kernel_stack_top);
	//__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"((u32)current_process->directory));

	//enter_usermode(current_process->regs->useresp);
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

static u32 cursor_ticks = 50;
static bool cursor_on = true;

void schedule(registers_state *regs) {
	(void)regs;
	process_t *next_proc;

	//if (!--cursor_ticks) {
	//	cursor_ticks = 50;
	//	if (cursor_on) {
	//		remove_cursor();
	//		cursor_on = false;
	//	} else {
	//		move_cursor();
	//		cursor_on = true;
	//	}
	//}

	for(process_t *p = proc_list; p != NULL; p = p->next) {
		if (p->timeout > 0 && p->timeout < 0xFFFFFFFF) {
			--p->timeout;
			if (!p->timeout) {
				wakeup_proc(p);
			}
		}
	}

	if (current_process->state == RUNNING && --current_process->timeslice > 0) {
		return;
	}

	for (;;) {
		i32 count = -1;
		for (process_t *p = proc_list; p != NULL; p = p->next) {
			if (p->state == RUNNING && p->timeslice > count) {
				count = p->timeslice;
				next_proc = p;
			}
		}
		if (count > 0) {
			break;
		}
		for (process_t *p = proc_list; p != NULL; p = p->next) {
			if (p->state == RUNNING) {
				p->timeslice = p->priority;
			}
		}
	}

	if (next_proc == current_process) {
		return;
	}

	task_switch(next_proc);
}

void task_switch(process_t *next_proc) {
	process_t *prev_proc = current_process;
	current_process = next_proc;

	tss_set_stack((u32)current_process->kernel_stack_top);
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"((u32)current_process->directory));
	switch_to(&prev_proc->context, current_process->context);
}
