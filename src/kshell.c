#include "kshell.h"
#include "stdio.h"
#include "memory.h"
#include "cmos.h"
#include "screen.h"
#include "string.h"
#include "debug.h"

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

#define NB_DOCUMENTED_COMMANDS 3

const i8 *commands[][NB_DOCUMENTED_COMMANDS] = {
	{"help", "Display information about OS shell commands"},
	{"date", "Print the system date and time"},
	{"clear", "Clear the terminal screen"},
};

u8 readline[READLINE_SIZE] = {0};
u32 readline_index = 0;
bool ctrl_mode = false;
// Scancode without info about pressed or released key
u8 raw_scancode = 0;

void help(const i8 *command) {
	const i8 *arg = 0;
	if (strlen(command) == 4) {
		arg = command;
	} else {
		arg = command + 5;
	}

	for (u8 i = 0; i < NB_DOCUMENTED_COMMANDS; ++i) {
		if (strncmp(arg, commands[i][0], strlen(commands[i][0])) == 0) {
			kprintf("%s - %s\n", arg, commands[i][1]);
			return;
		}
	}

	kprintf("No help for this command\n");
}

void date() {
	cmos_rtc_t date_and_time;

	date_and_time = cmos_read_rtc();	

	kprintf("%d/", date_and_time.year);
	if (date_and_time.month < 10) {
		kprintf("0");
	}
	kprintf("%d/", date_and_time.month);
	if (date_and_time.day < 10) {
		kprintf("0");
	}
	kprintf("%d ", date_and_time.day);
	if (date_and_time.hours < 10) {
		kprintf("0");
	}
	kprintf("%d:", date_and_time.hours);
	if (date_and_time.minutes < 10) {
		kprintf("0");
	}
	kprintf("%d:", date_and_time.minutes);
	if (date_and_time.seconds < 10) {
		kprintf("0");
	}
	kprintf("%d", date_and_time.seconds);
	kprintf("\n");
}

void clear() {
	screen_clear();
}

void run_command(const i8 *command) {
	if (*command == 0) {
		return;
	}

	if (strncmp(command, "help", 4) == 0) {
		help(command);
	} else if (strncmp(command, "date", 4) == 0) {
		date();
	} else if (strncmp(command, "clear", 5) == 0) {
		clear();
	} else {
		kprintf("Invalid command\n");
	}
}

void reset_readline() {
	readline_index = 0;
	memset((void *)readline, 0, READLINE_SIZE);
	kprintf(PROMPT);
}

void kshell(u8 scancode) {
	raw_scancode = scancode & ~KEYBOARD_RELEASE;
	switch (raw_scancode) {
		case KEY_LCTRL:
			if (KEY_IS_PRESSED(scancode)) {
				ctrl_mode = true;
			} else {
				ctrl_mode = false;
			}
			break;
		case KEY_BACKSPACE:
			if (KEY_IS_PRESSED(scancode)) {
				if (readline_index > 0) {
					kprintf("\b");
					--readline_index;
					readline[readline_index] = 0;
				}
			}
			break;
		case KEY_ENTER:
			if (KEY_IS_PRESSED(scancode)) {
				kprintf("\n");
				run_command((const i8*) readline);
				reset_readline();
			}
			break;
		case KEY_TAB:
			if (KEY_IS_PRESSED(scancode)) {
				kprintf("  ");
				readline[readline_index++] = ' ';
				readline[readline_index++] = ' ';
			}
			break;
		default:
			if (KEY_IS_PRESSED(scancode)) {
				u8 c = keyboard_layout_us[0][raw_scancode];
				if (!c) {
					return;
				}

				if (ctrl_mode) {
					if (c == 'c') {
						readline[readline_index] = '^';
						readline[readline_index] = 'C';
						kprintf("^C\n");
						kprintf(PROMPT);
					} else if (c == 'l') {
						clear();
						reset_readline();
					}
				} else {
					kprintf("%c", c);
					readline[readline_index++] = c;
				}
			}
			break;
	}
}
