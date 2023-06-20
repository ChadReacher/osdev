#include "port.h"

u8 inportb(u16 port) {
	u8 result;
	__asm__ volatile ("in %%dx, %%al" : "=a" (result) : "d" (port));
	return result;
}

void outportb(u16 port, u8 data) {
	__asm__ volatile ("out %%al, %%dx" : : "a" (data), "d" (port));
}

u16 inportw(u16 port) {
	u16 result;
	__asm__ volatile ("in %%dx, %%ax" : "=a" (result) : "d" (port));
	return result;
}

void outportw(u16 port, u16 data) {
	__asm__ volatile ("out %%ax, %%dx" : : "a" (data), "d" (port));
}
