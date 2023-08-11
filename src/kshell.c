#include "kshell.h"
#include "stdio.h"
#include "memory.h"
#include "cmos.h"
#include "screen.h"
#include "string.h"
#include "debug.h"
#include "test.h"
#include "heap.h"
#include "vfs.h"

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

static i8 *cwd = "/";

#define NB_DOCUMENTED_COMMANDS 5

const i8 *commands[][NB_DOCUMENTED_COMMANDS] = {
	{"help", "Display information about OS shell commands"},
	{"date", "Print the system date and time"},
	{"clear", "Clear the terminal screen"},
	{"selftest", "Run test suite"},
	{"ls", "list directories and files"},
};

i8 readline[READLINE_SIZE] = {0};
i8 last_readline[READLINE_SIZE] = {0};
u32 readline_index = 0;
// Scancode without info about pressed or released key
u8 raw_scancode = 0;
bool ctrl_mode = false;
bool shift_mode = false;
bool capslock_mode = false;

void help(const i8 *command) {
	if (strlen(command) == 4) {
		for (u8 i = 0; i < NB_DOCUMENTED_COMMANDS; ++i) {
			kprintf("%s - %s\n", commands[i][0], commands[i][1]);
		}
		return;
	}
	const i8 *arg = command + 5;
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

void selftest() {
	kprintf("OS selftest\n");
	kprintf("\n[Interrupts]\n");
	kprintf("  Invoking breakpoint interrupt:\n");
	__asm__ ("int3");
	kprintf("\n[Syscalls]\n");
	kprintf("  Invoking syscalls:\n");
	test("kernel shell");
	kprintf("\nEverything is good\n");
}

void ls() {
	u32 dir_entry_idx = 0;
	vfs_node_t *vfs_node = vfs_get_node(cwd);
	dirent *dir_entry;
	while (true) {
		dir_entry = vfs_readdir(vfs_node, dir_entry_idx);
		if (!dir_entry) {
			break;
		}
		kprintf("%s ", dir_entry->name);
		++dir_entry_idx;
		free(dir_entry);
	}
	kprintf("\n");
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
	} else if (strncmp(command, "selftest", 8) == 0) {
		selftest();
	} else if (strncmp(command, "ls", 2) == 0) {
		ls();
	} else {
		kprintf("Invalid command\n");
	}
}

void reset_readline() {
	readline_index = 0;
	memset((void *)readline, 0, READLINE_SIZE);
}

void kshell() {
	kprintf(cwd);
	kprintf(PROMPT);
	for (;;) {
		kshell_run(keyboard_get_last_scancode());
	}
}

void kshell_run(u8 scancode) {
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
				strcpy(last_readline, readline);
				reset_readline();
				kprintf(cwd);
				kprintf(PROMPT);
			}
			break;
		case KEY_TAB:
			if (KEY_IS_PRESSED(scancode)) {
				kprintf("  ");
				readline[readline_index++] = ' ';
				readline[readline_index++] = ' ';
			}
			break;
		case KEY_LSHIFT:
		case KEY_RSHIFT:
			if (KEY_IS_PRESSED(scancode)) {
				shift_mode = true;
			} else {
				shift_mode = false;
			}
			break;
		case KEY_CAPSLOCK:
			if (KEY_IS_PRESSED(scancode)) {
				capslock_mode = !capslock_mode;
			}			
			break;
		case UP_ARROW:
			if (KEY_IS_PRESSED(scancode)) {
				reset_readline();
				strcpy(readline, last_readline);
				kprintf(readline);
				readline_index = strlen(readline);
			}
			break;
		default:
			if (KEY_IS_PRESSED(scancode)) {
				u8 c = keyboard_layout_us[(shift_mode ^ capslock_mode) ? 1 : 0][raw_scancode];
				if (!c) {
					return;
				}

				if (ctrl_mode) {
					if (c == 'c') {
						reset_readline();
						kprintf("^C\n");
						kprintf(cwd);
						kprintf(PROMPT);
					} else if (c == 'l') {
						clear();
						reset_readline();
						kprintf(cwd);
						kprintf(PROMPT);
					}
				} else {
					kprintf("%c", c);
					readline[readline_index++] = c;
				}
			}
			break;
	}
}
