#include <scheduler.h>
#include <tss.h>
#include <queue.h>
#include <debug.h>
#include <syscall.h>
#include <stdlib.h>
#include <string.h>
#include <heap.h>
#include <syscall.h>


extern u32 ticks;
extern void switch_to(context_t **old_context, context_t *new_context);

queue_t *ready_queue;
queue_t *stopped_queue;
queue_t *procs;
process_t *current_process = NULL;
process_t *init_process = NULL;
process_t *idle_process = NULL;

void task_switch(process_t *next_proc);

void scheduler_init() {
	ready_queue = queue_new();
	stopped_queue = queue_new();
	procs = queue_new();
}

process_t *get_proc_by_id(u32 pid) {
	queue_node_t *node = procs->head;
	for (u32 i = 0; i < procs->len; ++i) {
		process_t *p = (process_t *)node->value;
		if (p->pid == pid) {
			return p;
		}
		node = node->next;
	}
	return NULL;
}

void schedule() {
	process_t *next_proc;

	DEBUG("%s", "Queue of ready processes:\r\n");
	queue_node_t *node = ready_queue->head;
	for (u32 i = 0; i < ready_queue->len; ++i) {
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
		DEBUG("Process(%p) with PID %d, next: %p, state: %s\r\n",
				p, p->pid, node->next, state);
		node = node->next;
	}


	DEBUG("Queue of all processes(%d):\r\n", procs->len);
	node = procs->head;
	for (u32 i = 0; i < procs->len; ++i) {
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
		DEBUG("Process(%p) with PID %d, next: %p, state: %s\r\n",
				p, p->pid, node->next, state);
		node = node->next;
	}

	// Stub implementation
	// TODO: Use separate data structure or organize it effectively
	node = procs->head;
	for (u32 i = 0; i < procs->len; ++i) {
		process_t *p = (process_t *)node->value;
		if (p->alarm && (u32)p->alarm < ticks) {
			send_signal(p, SIGALRM);
			p->alarm = 0;
		}
		if (p->sigpending != 0 && p->state == INTERRUPTIBLE) {
			p->state = RUNNING;
			queue_enqueue(ready_queue, p);
		}
		node = node->next;
	}

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

// Find first non-blocked signal
static i32 sigget(sigset_t *sigpend, const sigset_t *sigmask) {
	i32 sig;

	for (sig = 0; sig < NSIG; ++sig) {
		if (sigismember(sigpend, sig) && !sigismember(sigmask, sig)) {
			break;
		}
	}
	if (sig == NSIG) {
		return -1;
	}
	return sig;
}


i32 handle_signal() {
	i32 sig = sigget(&current_process->sigpending, &current_process->sigmask);
	if (sig <= 0) {
		return -1;
	}
	sigdelset(&current_process->sigpending, sig);
	sigaction_t *action = &(current_process->signals[sig]);
	if (current_process->pid == 1) {
		return 0;
	}

	if (action->sa_handler == SIG_DFL) {
		if (sig == SIGSTOP || sig == SIGTSTP || sig == SIGTTIN || sig == SIGTTOU) {
			if (!(current_process->parent->signals[SIGCHLD].sa_flags & SA_NOCLDSTOP)) {
				send_signal(current_process->parent, SIGCHLD);
			}
			current_process->state = STOPPED;
			current_process->exit_code = sig;
			queue_enqueue(stopped_queue, current_process);
			schedule();
			return 0;
		} else if (sig == SIGCHLD || sig == SIGCONT) {
			return 0;
		} else {
			syscall_exit(sig);
			// TODO: Abnormal termination of the process
		}
	} else if (action->sa_handler == SIG_IGN) {
		return 0;
	}

	memcpy(&current_process->old_sigmask, &current_process->sigmask, sizeof(sigset_t));

	current_process->signal_old_regs = *(current_process->regs);
	current_process->regs->eip = (u32)action->sa_handler;
	u32 *esp = (u32 *)current_process->regs->useresp;
	++esp;
	*esp = sig;
	--esp;
	*esp = (u32)current_process->sigreturn;
	return 0;
}

u32 send_signal(process_t *proc, i32 sig) {
	// Handle generating signal
	if (sig == SIGSTOP || sig == SIGTSTP || sig == SIGTTIN || sig == SIGTTOU) {
		sigdelset(&proc->sigpending, SIGCONT);
	} else if (sig == SIGCONT) {
		sigdelset(&proc->sigpending, SIGSTOP);
		sigdelset(&proc->sigpending, SIGTSTP);
		sigdelset(&proc->sigpending, SIGTTIN);
		sigdelset(&proc->sigpending, SIGTTOU);

		//TODO: Implement it properly or easier and simpler
		queue_node_t *node = stopped_queue->head;
		for (u32 i = 0; i < stopped_queue->len; ++i) {
			process_t *p = (process_t *)node->value;
			if (p->pid == proc->pid) {
				if (node->prev == NULL) {
					ready_queue->head = node->next;
					free(node);
					break;
				} else {
					node->prev->next = node->next;
					node->next->prev = node->prev;
					free(node);
					break;
				}
			}
			node = node->next;
		}
		--stopped_queue->len;
		proc->state = RUNNING;
		queue_enqueue(ready_queue, proc);
	}

	// Send the signal
	
	// Check if the signal is blocked
	// TODO: Don't understand
	if (!sigismember(&proc->sigmask, sig)) {
		if (proc->signals[sig].sa_handler == SIG_IGN && sig != SIGCHLD) {
			return 0;
		}
	}

	// Invalid state
	if (proc->state == ZOMBIE) {
		return -1;
	}

	sigaddset(&proc->sigpending, sig);
	return 0;
}
