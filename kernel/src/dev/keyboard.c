#include <keyboard.h>
#include <stdlib.h>
#include <port.h>
#include <screen.h>
#include <panic.h>
#include <tty.h>

#define RING_BUFFER_SIZE 8

extern struct tty_struct tty_table[];

/* Scancode without info about pressed or released key */
u32 leds = 0;
bool alt_mode = 0;
bool altgr_mode = 0;
bool ctrl_mode = 0;
bool shift_mode = 0;
bool capslock_mode = 0;

static u8 key_map[128] = {
	0, 27,							/* Null, Escape */
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
	127, '\t',						/* Backspace, tab */
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
	'\n', 0,						/* Enter, Left CTRL */
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
	0,								/* left shift */
	'\\', 
	'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
	0,								/* right shift */
	'*',							/* asterisk(numpad) */
	0,								/* left alt */
	' ',					
	0,								/* capslock, */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* F1 - F10 */
	0, 0,							/* NumLock, ScrollLock */
	0, 0, 0, '-',					/* 7(Home), 8, 9(PgUp), minus */
	0, 0, 0, '+', 					/* 4(left), 5(mid), 6(right), plus */
	0, 0, 0, 0, 0, 0,				/* 1(End), 2, 3(PgDn), 0, .(Del) */
	0, 0, 0,						/* NULL... */
	0, 0,							/* F11, F12 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static u8 shift_map[128] = {
	0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
	127, '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
	'\n', 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
	0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
	0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+',
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void put_queue(i8 ch) {
	u32 new_head;
	struct tty_queue *qp = &tty_table[0].input;

	qp->buf[qp->head] = ch;
	if ((new_head = (qp->head + 1) & (TTY_QUEUE_BUF_SZ - 1)) != qp->tail) {
		qp->head = new_head;
	}
	if (qp->process != NULL) {
		qp->process->state = RUNNING;
	}
}

static i8 ext_key = 0;

static void keyboard_handler() {
	u8 status, scancode, key;

	status = port_inb(KEYBOARD_STATUS_PORT);
	/* Is output buffer full? */
	if (!status & 0x01) {
		return;
	}
	scancode = port_inb(KEYBOARD_DATA_PORT);
	/* scancode is extended byte? */
	if (scancode == 0xE0) {
		debug("extkey\r\n");
		ext_key = 1;
		return;
	}

	/* If the key has been released */
	if (scancode & 0x80) {
		switch (scancode & 0x7F) {
			case KEY_LALT:
				debug("alt\r\n");
				if (!ext_key) {
					alt_mode = 0;
				} else {
					altgr_mode = 0;
				}
				break;
			case KEY_LCTRL:
				debug("ctrl\r\n");
				ctrl_mode = 0;
				break;
			case KEY_LSHIFT:
			case KEY_RSHIFT:
				debug("shift\r\n");
				if (!ext_key) {
					shift_mode = 0;
				}
				break;
			case KEY_CAPSLOCK:
			case KEY_NUMBERLOCK:
			case KEY_SCROLLLOCK:
				debug("capslock or numberlock or scrolllock\r\n");
				leds = 0;
				break;
		}
		ext_key = 0;
		return;
	}

	switch (scancode & 0x7F) {
		case KEY_CAPSLOCK:
			leds = 1;
			return;
		case KEY_NUMBERLOCK:
			leds = 1;
			return;
		case KEY_SCROLLLOCK:
			leds = 1;
			return;
		case KEY_LALT:
			if (!ext_key) {
				alt_mode = 1;
			} else {
				altgr_mode = 1;
			}
			return;
		case KEY_LCTRL:
			ctrl_mode = 1;
			return;
		case KEY_LSHIFT:
		case KEY_RSHIFT:
			shift_mode = 1;
			ext_key = 0;
			return;
	}
	debug("begin --------\r\n");
	debug("scancode - %x, %x\r\n", scancode, scancode & 0x7F);
	debug("end   --------\r\n");


	if (alt_mode) {
		key = 0;
		debug("keyboard_handler: alt_mode unimplemented\r\n"); 
		/*key = alt_map[scancode & 0x7F];*/
	} else if (shift_mode || ctrl_mode || capslock_mode) {
		key = shift_map[scancode & 0x7F];
		if (ctrl_mode) {
			key &= 0x1F;
		}
	} else {
		key = key_map[scancode & 0x7F];
	}
	if (key == 0) {
		return;
	}
	put_queue(key);
	do_cook(&tty_table[0]);
}

void keyboard_init() {
	register_interrupt_handler(IRQ1, keyboard_handler);
	debug("%s", "Keyboard has been initialized\r\n");
}
