#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

#define COM1 0x3F8

void serial_init();
void write_string_serial(i8 *str);
void serial_printf(i8 *str, ...);

#endif
