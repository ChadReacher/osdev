#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

#define COM1 0x3F8

void serial_init(void);
void write_serial(i8 *str);
i8 read_serial(void);

#endif
