#include "types.h"
#include "isr.h"
#include "timer.h"
#include "keyboard.h"
#include "screen.h"
#include "stdio.h"
#include "serial.h"
#include "debug.h"

__attribute__ ((section ("kernel_entry"))) void _start() {
	clear_screen();

	int result = init_serial();
	if (result == 1) {
		kprintf("Could not initiliaze serial port");
		for (;;) {}
	}
	DEBUG("%s has started\r\n", "OS");

	init_isr();
	init_timer(50);
	init_keyboard();

	__asm__ ("sti");

	for (;;) {}
}
