#ifndef PORT_H
#define PORT_H

#include "types.h"

static inline u8 port_inb(u16 port) {
	u8 res;
	__asm__ __volatile__ ("in %%dx, %%al" : "=a"(res) : "d"(port));
	return res;
}

static inline void port_outb(u16 port, u8 data) {
	__asm__ __volatile__ ("out %%al, %%dx" : : "a"(data), "d"(port));
}

#endif
