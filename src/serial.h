#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

#define COM1 0x3F8

i32 init_serial();
i32 serial_received();
i8 read_serial();
i32 is_transmit_empty();
void serial_write(i8 ch);
void serial_print(i8 *str);
void serial_printf(i8 *str, ...);

#endif
