#include <keyboard.h>
#include <stdlib.h>
#include <port.h>
#include <screen.h>
#include <debug.h>

#define RING_BUFFER_SIZE 8

// Scancode without info about pressed or released key
bool ctrl_mode = false;
bool shift_mode = false;
bool capslock_mode = false;

static u8 keyboard_layout_us[2][128] = {
	// When SHIFT is NOT pressed
	{
		KEY_NULL, KEY_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
		'-', '=', KEY_BACKSPACE, '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u',
		'i', 'o', 'p', '[', ']', '\n', KEY_LCTRL, 'a', 's', 'd', 'f', 'g',
		'h', 'j', 'k', 'l', ';', '\'', '`', KEY_LSHIFT, '\\', 'z', 'x', 'c',
		'v', 'b', 'n', 'm', ',', '.', '/', KEY_RSHIFT, '*', KEY_LALT, ' ',
		KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8,
		KEY_F9, KEY_F10, KEY_NUMBERLOCK, KEY_SCROLLLOCK, '7', '8',
		'9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.', 0, 0, 0, KEY_F11,
		KEY_F12,
	}, 
	// When SHIFT IS pressed
	{
		KEY_NULL, KEY_ESC, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
		'_', '+', KEY_BACKSPACE, '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U',
		'I', 'O', 'P', '{', '}', '\n', KEY_LCTRL, 'A', 'S', 'D', 'F', 'G',
		'H', 'J', 'K', 'L', ':', '"', '~', KEY_LSHIFT, '|', 'Z', 'X', 'C',
		'V', 'B', 'N', 'M', '<', '>', '?', KEY_RSHIFT, '*', KEY_LALT, ' ',
		KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8,
		KEY_F9, KEY_F10, KEY_NUMBERLOCK, KEY_SCROLLLOCK, '7', '8',
		'9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.', 0, 0, 0, KEY_F11,
		KEY_F12,
	}
};

u8 scancodes[RING_BUFFER_SIZE];
u8 buffer_len = 0;
u8 read_idx = 0;
u8 write_idx = 0;

u8 keyboard_getchar() {
	u8 scancode = keyboard_get_scancode();
	u8 raw_scancode = scancode & ~KEYBOARD_RELEASE;
	
	switch (raw_scancode) {
		case KEY_LCTRL:
			if (KEY_IS_PRESSED(scancode)) {
				ctrl_mode = true;
			} else {
				ctrl_mode = false;
			}
			return 0;
		case KEY_BACKSPACE:
			if (KEY_IS_PRESSED(scancode)) {
				return '\b';
			}
			return 0;
			break;
		case KEY_ENTER:
			if (KEY_IS_PRESSED(scancode)) {
				return '\n';
			}
			return 0;
			break;
		case KEY_TAB:
			if (KEY_IS_PRESSED(scancode)) {
				return '\t';
			}
			return 0;
			break;
		case KEY_LSHIFT:
		case KEY_RSHIFT:
			if (KEY_IS_PRESSED(scancode)) {
				shift_mode = true;
			} else {
				shift_mode = false;
			}
			return 0;
			break;
		case KEY_CAPSLOCK:
			if (KEY_IS_PRESSED(scancode)) {
				capslock_mode = !capslock_mode;
			}			
			return 0;
			break;
		case UP_ARROW:
		case DOWN_ARROW:
		case LEFT_ARROW:
		case RIGHT_ARROW:
			//if (KEY_IS_PRESSED(scancode)) {
			//	reset_readline();
			//	strcpy(readline, last_readline);
			//	kprintf(readline);
			//	readline_index = strlen(readline);
			//}
			return 0;
			break;
		default:
			if (KEY_IS_PRESSED(scancode)) {
				u8 c = keyboard_layout_us[(shift_mode ^ capslock_mode) ? 1 : 0][raw_scancode];
				if (!c) {
					return 0;
				}

				if (ctrl_mode) {
					return c;
					//if (c == 'c') {
					//	reset_readline();
					//	kprintf("^C\n");
					//	kprintf(cwd);
					//	kprintf(PROMPT);
					//} else if (c == 'l') {
					//	clear();
					//	reset_readline();
					//	kprintf(cwd);
					//	kprintf(PROMPT);
					//}
				} else {
					return c;
				}
			}
			return 0;
			break;
	}

}

u8 keyboard_get_scancode() {
	if (buffer_len == 0) {
		return 0;
	}
	u8 scancode = scancodes[read_idx++];
	--buffer_len;

	if (read_idx == RING_BUFFER_SIZE) {
		read_idx = 0;
	}

	return scancode;
}

static void keyboard_handler() {
	u8 status;

	status = port_inb(KEYBOARD_STATUS_PORT);
	if (status & 0x01) { // Is output buffer full?
		u8 scancode = port_inb(KEYBOARD_DATA_PORT);

		if (buffer_len == RING_BUFFER_SIZE) {
			DEBUG("%s", "Ring buffer is full.\r\n");
			return;
		}

		scancodes[write_idx++] = scancode;
		++buffer_len;

		if (write_idx == RING_BUFFER_SIZE) {
			write_idx = 0;
		}
	}
}

void keyboard_init() {
	register_interrupt_handler(IRQ1, keyboard_handler);
	DEBUG("%s", "Keyboard has been initialized\r\n");
}
