#include <tty.h>
#include <process.h>
#include <queue.h>
#include <scheduler.h>
#include <panic.h>
#include <errno.h>
#include <signal.h> 

void console_write(struct tty_struct *tty);
i32 is_orphaned_pgrp(i32 pgrp);

extern process_t *current_process;
extern queue_t *ready_queue;

i8 ttyq_getchar(struct tty_queue *q) {
	i8 c = q->buf[q->tail];
	q->tail = (q->tail + 1) & (TTY_QUEUE_BUF_SZ - 1);
	return c;
}

static void ttyq_putchar(struct tty_queue *q, i8 c) {
	q->buf[q->head] = c;
	q->head = (q->head + 1) & (TTY_QUEUE_BUF_SZ - 1);
}

struct tty_struct tty_table[] = {
	{
		{
			0,
			OPOST | ONLCR,
			0,
			IXON | ICANON | ECHO | ISIG,
			INIT_C_CC
		}, /* termios */
		0, /* initial pgrp */
		0, /* initially not stopped */
		console_write, /* write */
		{ 0, 0, 0, NULL, ""}, /* input */
		{ 0, 0, 0, NULL, ""}, /* output */
		{ 0, 0, 0, NULL, ""} /* cooked */
	},
};

void interruptible_sleep_on(struct _process **p) {
	process_t *tmp;
	if (!p) {
		return;
	}
	tmp = *p;
	*p = current_process;
repeat:
	current_process->state = INTERRUPTIBLE;
	schedule();
	if (*p && *p != current_process) {
		debug("i'm here!!!!!!!!!!!!!!!!!\r\n");
		(**p).state = RUNNING;
		goto repeat;
	}
	*p = NULL;
	if (tmp) {
		debug("LET's ASSIGN RUNNING STATE\r\n");
		tmp->state = RUNNING;
	}
}

void sleep_if_full(struct tty_queue *q) {
	if (!IS_FULL(*q)) {
		return;
	}
	while (!current_process->sigpending && LEFT_CHARS(*q) < 128) {
		interruptible_sleep_on(&q->process);
	}
}

i32 tty_read(u32 minor, i8 *buf, i32 count) {
	struct tty_struct *tty;
	i8 c, *b = buf;

	if (minor > 1 || count < 0) {
		return -1;
	}
	tty = &tty_table[minor];
	while (count > 0) {
		if (current_process->sigpending) {
			break;
		}
		if (EMPTY(tty->cooked) || ((tty->termios.c_lflag & ICANON) &&
				!tty->cooked.count && !IS_FULL(tty->input))) {
			interruptible_sleep_on(&tty->cooked.process);
			continue;
		}
		do {
			c = ttyq_getchar(&tty->cooked);
			if (c == EOF_CHAR(tty) || c == 10) {
				--tty->cooked.count;
			}
			if (c == EOF_CHAR(tty) && (tty->termios.c_lflag & ICANON)) {
				return b - buf;
			} else {
				*b = c;
				++b;
				if (!--count) {
					break;
				}
			}
		} while (count > 0 && !EMPTY(tty->cooked));
		if (tty->termios.c_lflag & ICANON) {
			if (b - buf) {
				break;
			}
		}
	}
	return b - buf;
}

i32 tty_write(u32 channel, i8 *buf, i32 count) {
	static i32 cr_flag = 0;
	struct tty_struct *tty;
	i8 c, *b = buf;

	if (channel > 1 || count < 0) {
		return -1;
	}
	tty = tty_table + channel;
	if ((tty->termios.c_lflag & TOSTOP) && current_process->tty == channel &&
			current_process->pgrp != tty->pgrp) {
		if (is_orphaned_pgrp(tty->pgrp)) {
			return -EIO;
		}
		kill_pgrp(current_process->pgrp, SIGTTOU);
		if (sigismember(&current_process->sigmask, SIGTTOU) ||
				current_process->signals[SIGTTOU].sa_handler == 1) {
			return -EIO;
		} else if (current_process->signals[SIGTTOU].sa_handler) {
			return -EINTR;
		} else {
			return -ERESTART;
		}
	}
	while (count > 0) {
		sleep_if_full(&tty->output);
		if (current_process->sigpending) {
			break;
		}
		while (count > 0 && !IS_FULL(tty->output)) {
			c = *b;
			if (tty->termios.c_oflag & OPOST) {
				if (c == '\n' && !cr_flag && 
						(tty->termios.c_oflag & ONLCR)) {
					cr_flag = 1;
					ttyq_putchar(&tty->output, 13);
					continue;
				}
			}
			++b;
			--count;
			cr_flag = 0;
			ttyq_putchar(&tty->output, c);
		}
		tty->write(tty);
		if (count > 0) {
			schedule();
		}
	}
	return b - buf;
}

extern queue_t *procs;
static void send_intr(struct tty_struct *tty, i32 signal) {
	i32 i, len;
	queue_node_t *node;
	process_t *p;

	if (tty->pgrp <= 0) {
		return;
	}
	node = procs->head; 
	len = procs->len;
	for (i = 0; i < len; ++i) {
		p = (process_t *)node->value;
		if (p->pgrp == tty->pgrp) {
			send_signal(p, signal);
		}
		node = node->next;
	}
}

void do_cook(struct tty_struct *tty) {
	i8 c;

	while (!EMPTY(tty->input) && !IS_FULL(tty->cooked)) {
		c = ttyq_getchar(&tty->input);
		if (tty->termios.c_iflag & ISTRIP) {
			c &= 0x7F;
		}
		if (c == 13) { 
			if (tty->termios.c_iflag & IGNCR) {
				continue;
			} else if (tty->termios.c_iflag & ICRNL) {
				c = 10;
			}
		} else if (c == 10 && (tty->termios.c_iflag & INLCR)) {
			c = 13;
		}
		if (tty->termios.c_lflag & ICANON) {
			if (c == KILL_CHAR(tty)) {
				while (!(EMPTY(tty->cooked) ||
						(c=LAST_CHAR(tty->cooked)) == 10 ||
						c == EOF_CHAR(tty))) {
					if (tty->termios.c_lflag & ECHO) {
						if (c < 32) {
							ttyq_putchar(&tty->output, 127);
						}
						ttyq_putchar(&tty->output, 127);
						tty->write(tty);
					}
					--tty->cooked.head;
				}
				continue;
			}
			if (c == ERASE_CHAR(tty)) {
				if (EMPTY(tty->cooked) ||
					(c = LAST_CHAR(tty->cooked)) == 10 ||
					c == EOF_CHAR(tty)) {
					continue;
				}
				if (tty->termios.c_lflag & ECHO) {
					if (c < 32) {
						ttyq_putchar(&tty->output, 127);
					}
					ttyq_putchar(&tty->output, 127);
					tty->write(tty);
				}
				--tty->cooked.head;
				continue;
			}
		}
		if (tty->termios.c_iflag & IXON) {
			if (c == STOP_CHAR(tty)) {
				tty->stopped = 1;
				continue;
			}
			if (c == START_CHAR(tty)) {
				tty->stopped = 0;
				continue;
			}
		}
		if (tty->termios.c_lflag & ISIG) {
			if (c == INTR_CHAR(tty)) {
				send_intr(tty, SIGINT);
				continue;
			} else if (c == QUIT_CHAR(tty)) {
				send_intr(tty, SIGQUIT);
				continue;
			} else if (c == SUSPEND_CHAR(tty)) {
				if (!is_orphaned_pgrp(tty->pgrp)) {
					kill_pgrp(tty->pgrp, SIGTSTP);
					continue;
				}
			}
		}
		if (c == 10 || c == EOF_CHAR(tty)) {
			++tty->cooked.count;
		}
		if (tty->termios.c_lflag & ECHO) {
			if (c == 10) {
				ttyq_putchar(&tty->output, 10);
				ttyq_putchar(&tty->output, 13);
			} else if (c < 32) {
				ttyq_putchar(&tty->output, '^');
				ttyq_putchar(&tty->output, c + 64);
			} else {
				ttyq_putchar(&tty->output, c);
			}
			tty->write(tty);
		}
		ttyq_putchar(&tty->cooked, c);
	}
	wake_up(&tty->cooked.process);
}

