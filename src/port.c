#include "port.h"

u8 port_inb(u16 port) {
	u8 result;
	__asm__ volatile ("in %%dx, %%al" : "=a" (result) : "d" (port));
	return result;
}

void port_outb(u16 port, u8 data) {
	__asm__ volatile ("out %%al, %%dx" : : "a" (data), "d" (port));
}
