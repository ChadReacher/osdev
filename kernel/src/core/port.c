#include <port.h>

u8 port_inb(u16 port) {
	u8 res;
	__asm__ volatile ("in %%dx, %%al" : "=a"(res) : "d"(port));
	return res;
}

void port_outb(u16 port, u8 data) {
	__asm__ volatile ("out %%al, %%dx" : : "a"(data), "d"(port));
}

u16 port_inw(u16 port) {
	u16 res;
	__asm__ volatile ("in %%dx, %%ax" : "=a"(res) : "d"(port));
	return res;
}

void port_outw(u16 port, u16 data) {
	__asm__ volatile ("out %%ax, %%dx" : : "a"(data), "d"(port));
}

u32 port_inl(u16 port) {
	u32 res;
	__asm__ volatile ("inl %%dx, %%eax" : "=a"(res) : "d"(port));
	return res;
}

void port_outl(u16 port, u32 data) {
	__asm__ volatile ("outl %%eax, %%dx" : : "d"(port), "a"(data));
}
