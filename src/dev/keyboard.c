#include <types.h>
#include <keyboard.h>
#include <stdio.h>
#include <stdlib.h>
#include <port.h>
#include <screen.h>
#include <debug.h>

#define RING_BUFFER_SIZE 8

u8 scancodes[RING_BUFFER_SIZE];
u8 buffer_len = 0;
u8 read_idx = 0;
u8 write_idx = 0;

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
