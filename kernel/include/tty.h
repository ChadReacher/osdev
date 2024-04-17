#ifndef TTY_H
#define TTY_H

#include "types.h"
#include "termios.h"
#include "process.h"

#define TTY_QUEUE_BUF_SZ 2048
struct tty_queue {
	u32 count;
	u32 head;
	u32 tail;
	struct proc *process;
	i8 buf[TTY_QUEUE_BUF_SZ];
};

struct tty_struct {
	struct termios termios;
	i32 pgrp;
	i32 stopped;
	void (*write)(struct tty_struct *tty);
	struct tty_queue input;
	struct tty_queue output;
	struct tty_queue cooked;
};

#define EMPTY(q) ((q).head == (q).tail)
#define LEFT_CHARS(q) (((q).tail - (q).head - 1) & (TTY_QUEUE_BUF_SZ - 1))
#define LAST_CHAR(q) ((q).buf[((q).head - 1) & (TTY_QUEUE_BUF_SZ - 1)])
#define IS_FULL(q) (!LEFT_CHARS(q))
#define CHARS(q) (((q).head - (q).tail) & (TTY_QUEUE_BUF_SZ - 1))

#define INTR_CHAR(tty) (tty->termios.c_cc[VINTR])
#define QUIT_CHAR(tty) (tty->termios.c_cc[VQUIT])
#define ERASE_CHAR(tty) (tty->termios.c_cc[VERASE])
#define KILL_CHAR(tty) (tty->termios.c_cc[VKILL])
#define EOF_CHAR(tty) (tty->termios.c_cc[VEOF])
#define START_CHAR(tty) (tty->termios.c_cc[VSTART])
#define SUSPEND_CHAR(tty) (tty->termios.c_cc[VSUSP])
#define STOP_CHAR(tty) (tty->termios.c_cc[VSTOP])


/* VINTR - 003, VQUIT - 034, VERASE - 177, VKILL - 025, VEOF  - 004,
 * VTIME - 000, VMIN  - 001, VSTART - 021, VSUSP - 032, VSTOP - 023, 
 * VEOL  - 003 */
#define INIT_C_CC "\003\034\177\025\004\0\1\021\032\023\0"


i32 tty_read(u16 channel, i8 *buf, i32 count);
i32 tty_write(u16 channel, i8 *buf, i32 count);
void do_cook(struct tty_struct *tty);
i8 ttyq_getchar(struct tty_queue *q);

#endif
