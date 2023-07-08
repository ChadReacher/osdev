#include "types.h"
#include "keyboard.h"
#include "stdio.h"
#include "k_stdlib.h"
#include "port.h"
#include "screen.h"
#include "debug.h"

u8 last_scancode = 0;
u8 last_read = 0;

u8 keyboard_get_last_scancode() {
	if (last_read == 1) {
		return 0;
	}
	DEBUG("last_scancode: %d\r\n", last_scancode);
	last_read = 1;
	return last_scancode;
}

static void keyboard_handler() {
	u8 scancode;

	scancode = port_inb(KEYBOARD_DATA_PORT);
	DEBUG("Received scancode: %d\r\n", scancode);	
	last_scancode = scancode;
	last_read = 0;
}

void keyboard_init() {
	register_interrupt_handler(IRQ1, keyboard_handler);
	DEBUG("%s", "Keyboard has been initialized\r\n");
}
