#include "types.h"
#include "keyboard.h"
#include "stdio.h"
#include "k_stdlib.h"
#include "port.h"
#include "screen.h"
#include "debug.h"

u8 last_scancode = 0;

u8 keyboard_get_last_scancode() {
	u8 scancode;

	scancode = last_scancode;
	last_scancode = 0;
	return scancode;
}

static void keyboard_handler() {
	u8 status;

	status = port_inb(KEYBOARD_STATUS_PORT);
	if (status & 0x01) { // Is output buffer full?
		last_scancode = port_inb(KEYBOARD_DATA_PORT);
		DEBUG("Received scancode: %d\r\n", last_scancode);	
	}
}

void keyboard_init() {
	register_interrupt_handler(IRQ1, keyboard_handler);
	DEBUG("%s", "Keyboard has been initialized\r\n");
}
