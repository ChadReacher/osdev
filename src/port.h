#ifndef PORT_H
#define PORT_H

#include "util.h"

u8 port_inb(u16 port);
void port_outb(u16 port, u8 data);

#endif
