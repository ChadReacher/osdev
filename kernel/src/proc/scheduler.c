#include <scheduler.h>
#include <tss.h>
#include <queue.h>
#include <syscall.h>
#include <stdlib.h>
#include <string.h>
#include <heap.h>
#include <syscall.h>
#include <process.h>
#include <panic.h>

extern u32 ticks;
extern void switch_to(context_t **old_context, context_t *new_context);

i32 need_resched = 0;
queue_t *ready_queue;
queue_t *stopped_queue;
queue_t *procs;
process_t *current_process = NULL;
process_t *init_process = NULL;
process_t *idle_process = NULL;

void task_switch(process_t *next_proc);
extern void irq_ret();
extern u32 next_pid;

static void cpu_idle() {
	while (1){ 
		/*
		__asm__ __volatile__("cli");
		kprintf("cpu idle\n");
		__asm__ __volatile__("sti");
		*/
		__asm__ __volatile__("hlt");
	}
}

void create_idle_process() {
	idle_process = malloc(sizeof(process_t));
	if (!idle_process) {
		panic("Failed to create 'idle' process\n");
	}
	queue_enqueue(procs, idle_process);
	memset(idle_process, 0, sizeof(process_t));
	idle_process->pid = next_pid++;
	idle_process->timeslice = 20;
	idle_process->state = RUNNING;
	idle_process->directory = virtual_to_physical((void *)0xFFFFF000);
	idle_process->kernel_stack_bottom = malloc(4096 *2);
	memset(idle_process->kernel_stack_bottom, 0, 4096 * 2);
	idle_process->regs = (registers_state *)
		(ALIGN_DOWN((u32)idle_process->kernel_stack_bottom + 4096 * 2 - 1, 4)
		 - sizeof(registers_state) + 4);
	idle_process->regs->eflags = 0x202;
	idle_process->regs->cs = 0x8;
	idle_process->regs->ds = 0x8;
	idle_process->regs->es = 0x8;
	idle_process->regs->fs = 0x8;
	idle_process->regs->gs = 0x8;
	idle_process->regs->eip = (u32)cpu_idle;
	idle_process->context = (context_t *)
		(ALIGN_DOWN((u32)idle_process->kernel_stack_bottom + 4096 * 2 - 1, 4)
		 - sizeof(registers_state) - sizeof(context_t) + 4);
	idle_process->context->eip = (u32)irq_ret;
	idle_process->kernel_stack_top = idle_process->context;
}

void scheduler_init() {
	ready_queue = queue_new();
	stopped_queue = queue_new();
	procs = queue_new();

	create_idle_process();

	current_process = init_process = malloc(sizeof(process_t));
	queue_enqueue(ready_queue, init_process);
	queue_enqueue(procs, init_process);
	memset(init_process, 0, sizeof(process_t));
	init_process->pid = next_pid++;
	init_process->timeslice = 20;
	init_process->state = RUNNING;
	init_process->kernel_stack_bottom = malloc(4096 * 2);
	memset(init_process->kernel_stack_bottom, 0, 4096 * 2);
	init_process->regs = (registers_state *)
		(ALIGN_DOWN((u32)init_process->kernel_stack_bottom + 4096 * 2 - 1, 4)
		 - sizeof(registers_state) + 4);
	init_process->context = (context_t *)
		(ALIGN_DOWN((u32)init_process->kernel_stack_bottom + 4096 * 2 - 1, 4)
		 - sizeof(registers_state) - sizeof(context_t) + 4);
}

process_t *get_proc_by_id(u32 pid) {
	queue_node_t *node = procs->head;
	u32 i;
	for (i = 0; i < procs->len; ++i) {
		process_t *p = (process_t *)node->value;
		if (p->pid == pid) {
			return p;
		}
		node = node->next;
	}
	return NULL;
}

void wake_up(process_t **p) {
	if (p && *p) {
		(**p).state = RUNNING;
		queue_enqueue(ready_queue, *p);
		*p = NULL;
		need_resched = 1;
	}
}

void goto_sleep(process_t **p) {
	process_t *tmp;
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
	u32 i;
	process_t *next_proc;
	queue_node_t *node;

	/*
	debug("Queue of ready processes:\r\n");
	node = ready_queue->head;
	for (i = 0; i < ready_queue->len; ++i) {
		process_t *p = (process_t *)node->value;
		char *state;
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
		}
		debug("Process(%p) with PID %d, next: %p, state: %s\r\n",
				p, p->pid, node->next, state);
		node = node->next;
	}


	debug("Queue of all processes(%d):\r\n", procs->len);
	node = procs->head;
	for (i = 0; i < procs->len; ++i) {
		process_t *p = (process_t *)node->value;
		char *state;
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
		}
		debug("Process(%p) with PID %d, next: %p, state: %s\r\n",
				p, p->pid, node->next, state);
		node = node->next;
	}*/

	/* Stub implementation */
	/* TODO: Use separate data structure or organize it effectively */
	node = procs->head;
	for (i = 0; i < procs->len; ++i) {
		process_t *p = (process_t *)node->value;
		if (p->alarm && (u32)p->alarm < ticks) {
			send_signal(p, SIGALRM);
			p->alarm = 0;
		}
		if (p->sleep && p->sleep < ticks) {
			p->state = RUNNING;
			p->sleep = 0;
			queue_enqueue(ready_queue, p);
		}
		if (p->sigpending != 0 && p->state == INTERRUPTIBLE) {
			p->state = RUNNING;
			queue_enqueue(ready_queue, p);
		}
		node = node->next;
	}

	need_resched = 0;
	if (!ready_queue->len) {
		if (current_process->state == RUNNING) {
			return;
		}
		next_proc = idle_process;
	} else {
		next_proc = queue_dequeue(ready_queue);
	}
	if (current_process != idle_process && current_process->state == RUNNING) {
		queue_enqueue(ready_queue, current_process);
	}
	task_switch(next_proc);
}

void task_switch(process_t *next_proc) {
	process_t *prev_proc = current_process;
	current_process = next_proc;

	tss_set_stack((u32)current_process->kernel_stack_top);
	__asm__ __volatile__ ("movl %%eax, %%cr3" 
            : 
            : "a"((u32)current_process->directory));
	switch_to(&prev_proc->context, current_process->context);
}

