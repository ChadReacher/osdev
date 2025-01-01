#include <tty.h>
#include <process.h>
#include <queue.h>
#include <scheduler.h>
#include <panic.h>
#include <errno.h>
#include <signal.h> 
#include <chr_dev.h>

void console_write(struct tty_struct *tty);
i32 is_orphaned_pgrp(i32 pgrp);
i32 kill_pgrp(i32 pgrp, i32 sig);

extern struct proc *procs[NR_PROCS];

i8 ttyq_getchar(struct tty_queue *q) {
	i8 c = q->buf[q->tail];
	q->tail = (q->tail + 1) & (TTY_QUEUE_BUF_SZ - 1);
	return c;
}

static void ttyq_putchar(struct tty_queue *q, i8 c) {
	q->buf[q->head] = c;
	q->head = (q->head + 1) & (TTY_QUEUE_BUF_SZ - 1);
}

struct file_ops tty_ops = {
	tty_open,
	tty_read,
    tty_write,
	NULL,
};

struct file_ops ttyx_ops = {
	ttyx_open,
	tty_read,
    tty_write,
	NULL,
};

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

i32 tty_open(struct vfs_inode *inode, struct file *fp) {
	u16 major, minor;
	major = MAJOR(inode->i_rdev);
	minor = MINOR(inode->i_rdev);

	if (current_process->tty < 0) {
		return -EPERM;
	}
	return 0;
}

i32 ttyx_open(struct vfs_inode *inode, struct file *fp) {
	u16 major, minor;
	major = MAJOR(inode->i_rdev);
	minor = MINOR(inode->i_rdev);

	if (current_process->leader && current_process->tty < 0) {
		current_process->tty = minor;
		tty_table[current_process->tty].pgrp = current_process->pgrp;
	}
	return 0;
}

void sleep_if_full(struct tty_queue *q) {
	if (!IS_FULL(*q)) {
		return;
	}
	while (!current_process->sigpending && LEFT_CHARS(*q) < 128) {
		process_sleep(&q->process);
	}
}

i32 tty_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count) {
	struct tty_struct *tty;
	i8 c, *b = buf;

	if (current_process->tty < 0) {
		return -EPERM;
	}
	i32 minor = MINOR(inode->i_rdev);
	if (minor > 1 || count < 0) {
		return -EINVAL;
	}
	tty = &tty_table[minor];
	while (count > 0) {
		if (current_process->sigpending) {
			break;
		}
		if (EMPTY(tty->cooked) || ((tty->termios.c_lflag & ICANON) &&
				!tty->cooked.count && !IS_FULL(tty->input))) {
			process_sleep(&tty->cooked.process);
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

i32 tty_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count) {
	static i32 cr_flag = 0;
	struct tty_struct *tty;
	i8 c, *b = buf;

	i32 minor = MINOR(inode->i_rdev);
	if (minor > 1 || count < 0) {
		return -EINVAL;
	}
	tty = tty_table + minor;
	if ((tty->termios.c_lflag & TOSTOP) && 
			current_process->tty == (i32)minor &&
			current_process->pgrp != tty->pgrp) {
		if (is_orphaned_pgrp(tty->pgrp)) {
			return -EIO;
		}
		kill_pgrp(current_process->pgrp, SIGTTOU);
		if (sigismember(&current_process->sigmask, SIGTTOU) ||
				(u32)current_process->signals[SIGTTOU].sa_handler == 1) {
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

static void send_intr(struct tty_struct *tty, i32 signal) {
	i32 i;
	struct proc *p;

	if (tty->pgrp <= 0) {
		return;
	}
	for (i = 0; i < NR_PROCS; ++i) {
		p = procs[i];
		if (!p) {
			continue;
		}
		if (p->pgrp == tty->pgrp) {
			send_signal(p, signal);
		}
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
	process_wakeup(&tty->cooked.process);
}

