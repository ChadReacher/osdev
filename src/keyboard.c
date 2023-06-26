#include "types.h"
#include "k_stdio.h"
#include "k_stdlib.h"
#include "port.h"
#include "isr.h"
#include "keyboard.h"
#include "screen.h"
#include "serial.h"

static bool ctrl = false;
static bool shift = false;
static bool break_code_active = false;

static u8 keyboard_layout_us[2][128] = {
	{
		KEY_NULL, KEY_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
		'-', '=', KEY_BACKSPACE, KEY_TAB, 'q', 'w', 'e', 'r', 't', 'y', 'u',
		'i', 'o', 'p', '[', ']', KEY_ENTER, KEY_LCTRL, 'a', 's', 'd', 'f', 'g',
		'h', 'j', 'k', 'l', ';', '\'', '`', KEY_LSHIFT, '\\', 'z', 'x', 'c',
		'v', 'b', 'n', 'm', ',', '.', '/', KEY_RSHIFT, '*', KEY_LALT, ' ',
		KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8,
		KEY_F9, KEY_F10, KEY_NUMBERLOCK, KEY_SCROLLLOCK, '7', '8',
		'9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.', 0, 0, 0, KEY_F11,
		KEY_F12,
	}, 
	{
		KEY_NULL, KEY_ESC, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
		'_', '+', KEY_BACKSPACE, KEY_TAB, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U',
		'I', 'O', 'P', '{', '}', KEY_ENTER, KEY_LCTRL, 'A', 'S', 'D', 'F', 'G',
		'H', 'J', 'K', 'L', ':', '"', '~', KEY_LSHIFT, '|', 'Z', 'X', 'C',
		'V', 'B', 'N', 'M', '<', '>', '?', KEY_RSHIFT, '*', KEY_LALT, ' ',
		KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8,
		KEY_F9, KEY_F10, KEY_NUMBERLOCK, KEY_SCROLLLOCK, '7', '8',
		'9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.', 0, 0, 0, KEY_F11,
		KEY_F12,
	}
};

static void keyboard_handler() {
	u8 scancode, character;

	scancode = port_inb(0x60);

	if (scancode == 0xE0) {
		break_code_active = true;
		return;
	} 

	if (KEY_IS_PRESSED(scancode)) {
		if (break_code_active) {
			if (scancode == LEFT_ARROW) {
				move_cursor_left();
			} else if (scancode == RIGHT_ARROW) {
				move_cursor_right();
			} else if (scancode == UP_ARROW) {
				move_cursor_up();
			} else if (scancode == DOWN_ARROW) {
				move_cursor_down();
			}
			break_code_active = false;
		} else {
			if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
				shift = true;
			} else if (scancode == KEY_LCTRL) {
				ctrl = true;
			} else if (scancode == KEY_BACKSPACE) {
				screen_backspace();
			} else if (scancode == KEY_LALT) {
				// skip for now
			} else {
				character = shift ? keyboard_layout_us[1][scancode &0x7F] : keyboard_layout_us[0][scancode &0x7F];
				kprintf("%c", character);
			}
		}
	} else if (KEY_IS_RELEASED (scancode)) {
		if (break_code_active) {
			break_code_active = false;
		} else {
			if (scancode == LEFT_SHIFT_RELEASED || scancode == RIGHT_SHIFT_RELEASED) {
				shift = false;
			}
		}
	}
}

void init_keyboard() {
	register_interrupt_handler(IRQ1, keyboard_handler);
}
