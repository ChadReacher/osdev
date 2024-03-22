#include "scheduler.h"
#include "queue.h"
#include "heap.h"
#include "string.h"
#include "errno.h"
#include "isr.h"


void do_exit(i32 code);

extern process_t *current_process;
extern queue_t *stopped_queue;
extern queue_t *ready_queue;

/* Find first non-blocked signal */
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

i32 handle_signal(registers_state *regs) {
	u32 *esp;
	sigaction_t *action;
	i32 sig;

	sig = sigget(&current_process->sigpending, &current_process->sigmask);
	if (sig <= 0) {
		return -1;
	}
	sigdelset(&current_process->sigpending, sig);
	action = &(current_process->signals[sig]);
	if ((i32)regs->eax == -ERESTART) {
		if ((u32)action->sa_handler > 1) {
			regs->eax = -EINTR;
		} else {
			regs->eax = regs->int_number;
			regs->eip -= 2;
		}
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
			do_exit(sig);
		}
	} else if (action->sa_handler == SIG_IGN) {
		return 0;
	}
	memcpy(&current_process->old_sigmask, &current_process->sigmask, sizeof(sigset_t));
	current_process->signal_old_regs = *(current_process->regs);
	regs->eip = (u32)action->sa_handler;
	esp = (u32*)regs->useresp;
	++esp;
	*esp = sig;
	--esp;
	*esp = (u32)current_process->sigreturn;
	return 0;
}

i32 send_signal(process_t *proc, i32 sig) {
	u32 i;
	if (!proc) {
		return -EINVAL;
	}
	if (!sig) {
		return -EINVAL;
	}
	if (current_process->euid != proc->euid && current_process->euid != 0) {
		return -EPERM;
	}

	if (sig == SIGSTOP || sig == SIGTSTP || sig == SIGTTIN || sig == SIGTTOU) {
		sigdelset(&proc->sigpending, SIGCONT);
	} else if (sig == SIGCONT) {
		queue_node_t *node;
		sigdelset(&proc->sigpending, SIGSTOP);
		sigdelset(&proc->sigpending, SIGTSTP);
		sigdelset(&proc->sigpending, SIGTTIN);
		sigdelset(&proc->sigpending, SIGTTOU);

		/*TODO: Implement it properly or easier and simpler */
		if (proc->state == STOPPED) {
			node = stopped_queue->head;
			for (i = 0; i < stopped_queue->len; ++i) {
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
			proc->exit_code = 0;
			queue_enqueue(ready_queue, proc);
		}
	}
	/* Send the signal */
	sigaddset(&proc->sigpending, sig);
	return 0;
}

