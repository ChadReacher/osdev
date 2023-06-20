#ifndef PORT_H
#define PORT_H

#include "../util.h"

u8 inportb(u16);
void outportb(u16, u8);
u16 inportw(u16);
void outportw(u16, u16);

#endif
