#include <scheduler.h>
#include <tss.h>
#include <syscall.h>
#include <stdlib.h>
#include <string.h>
#include <heap.h>
#include <syscall.h>
#include <process.h>
#include <panic.h>

extern u32 ticks;
extern u32 next_pid;
extern void irq_ret();
extern void switch_to(struct context **old_context, struct context *new_context);

i32 need_resched = 0;
struct proc *procs[NR_PROCS];
struct proc *current_process = NULL;

void task_switch(struct proc *next_proc);

static void cpu_idle() {
	while (1){ 
		__asm__ volatile ("hlt");
	}
}

struct proc *get_proc_by_id(i32 pid) {
	if (pid < 0 || pid > NR_PROCS) {
		return NULL;
	}
	return procs[pid];
}

int get_free_proc() {
	int i;
	for (i = 0; i < NR_PROCS; ++i) {
		if (procs[i] == NULL) {
			return i;
		}
	}
	return -1;
}

void wake_up(struct proc **p) {
	if (p && *p) {
		(**p).state = RUNNING;
		*p = NULL;
		need_resched = 1;
	}
}

void goto_sleep(struct proc **p) {
	struct proc *tmp;
	if (!p) {
		return;
	}
	tmp = *p;
	*p = current_process;
	current_process->state = INTERRUPTIBLE;
	schedule();
	debug("after sleep\n");
	if (tmp) {
		tmp->state = RUNNING;
	}
}

void schedule() {
	int i, count;
	struct proc *next_proc, *p;

	debug("Queue of all processes:\r\n");
	for (i = 0; i < NR_PROCS; ++i) {
		char *state;
		struct proc *p = procs[i];
		if (!p) {
			continue;
		}
		switch (p->state) {
			case RUNNING:
				state = "RUNNING";
				break;
			case ZOMBIE:
				state = "ZOMBIE";
				break;
			case INTERRUPTIBLE:
				state = "INTERRUPTIBLE";
				break;
			case STOPPED:
				state = "STOPPED";
				break;
			default:
				state = "UNKNOWN";
				break;
		}
		debug("Process(%p) with PID %d, state: %s\r\n",
				p, p->pid, state);
	}

	for (i = 1; i < NR_PROCS; ++i) {
		struct proc *p = procs[i];
		if (!p) {
			continue;
		}
		if (p->alarm && (u32)p->alarm < ticks) {
			send_signal(p, SIGALRM);
			p->alarm = 0;
		}
		if (p->sleep && p->sleep < ticks) {
			p->sleep = 0;
			if (p->state == INTERRUPTIBLE) {
				p->state = RUNNING;
			}
		}
		if (p->sigpending != 0 && p->state == INTERRUPTIBLE) {
			p->sleep = 0;
			p->state = RUNNING;
		}
	}

	need_resched = 0;
	while (1) {
		count = -1;
		next_proc = procs[0];
		for (i = 1; i < NR_PROCS; ++i) {
			p = procs[i];
			if (!p) {
				continue;
			}
			if (p->state == RUNNING && p->timeslice > count) {
				next_proc = p;
				count = p->timeslice;
			}
		}	
		if (count) {
			break;
		}
		for (i = 1; i < NR_PROCS; ++i) {
			p = procs[i];
			if (p) {
				p->timeslice = 20;
			}
		}	
	}
	
	if (next_proc != current_process) {
		task_switch(next_proc);
	}
}

void task_switch(struct proc *next_proc) {
	struct proc *prev_proc = current_process;
	current_process = next_proc;

	tss_set_stack((u32)current_process->kernel_stack_top);
	__asm__ volatile ("movl %%eax, %%cr3" 
            : 
            : "a"((u32)current_process->directory));
	switch_to(&prev_proc->context, current_process->context);
}

void create_idle_process() {
	struct proc *idle_process = procs[0] = malloc(sizeof(struct proc));
	memset(idle_process, 0, sizeof(struct proc));

	idle_process->pid = next_pid++;
	idle_process->timeslice = 20;
	idle_process->state = RUNNING;
	idle_process->directory = virtual_to_physical((void *)0xFFFFF000);
	idle_process->kernel_stack_bottom = malloc(4096 *2);
	memset(idle_process->kernel_stack_bottom, 0, 4096 * 2);
	idle_process->regs = (struct registers_state *)
		(ALIGN_DOWN((u32)idle_process->kernel_stack_bottom + 4096 * 2 - 1, 4)
		 - sizeof(struct registers_state) + 4);
	idle_process->regs->eflags = 0x202;
	idle_process->regs->cs = 0x8;
	idle_process->regs->ds = 0x8;
	idle_process->regs->es = 0x8;
	idle_process->regs->fs = 0x8;
	idle_process->regs->gs = 0x8;
	idle_process->regs->eip = (u32)cpu_idle;
	idle_process->context = (struct context *)
		(ALIGN_DOWN((u32)idle_process->kernel_stack_bottom + 4096 * 2 - 1, 4)
		 - sizeof(struct registers_state) - sizeof(struct context) + 4);
	idle_process->context->eip = (u32)irq_ret;
	idle_process->kernel_stack_top = idle_process->context;
}

void scheduler_init() {
	struct proc *init_process;

	create_idle_process();

	init_process = current_process = procs[1] = malloc(sizeof(struct proc));
	memset(init_process, 0, sizeof(struct proc));
	init_process->pid = next_pid++;
	init_process->timeslice = 20;
	init_process->state = RUNNING;
	init_process->kernel_stack_bottom = malloc(4096 * 2);
	memset(init_process->kernel_stack_bottom, 0, 4096 * 2);
	init_process->regs = (struct registers_state *)
		(ALIGN_DOWN((u32)init_process->kernel_stack_bottom + 4096 * 2 - 1, 4)
		 - sizeof(struct registers_state) + 4);
	init_process->context = (struct context *)
		(ALIGN_DOWN((u32)init_process->kernel_stack_bottom + 4096 * 2 - 1, 4)
		 - sizeof(struct registers_state) - sizeof(struct context) + 4);
}
