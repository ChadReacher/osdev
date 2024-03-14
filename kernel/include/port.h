#ifndef PORT_H
#define PORT_H

#include "types.h"

u8 port_inb(u16 port);
void port_outb(u16 port, u8 data);
u16 port_inw(u16 port);
void port_outw(u16 port, u16 data);
u32 port_inl(u16 port);
void port_outl(u16 port, u32 data);

#endif
