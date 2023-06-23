#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

#define COM1 0x3F8

i32 init_serial();
i32 serial_received();
i8 read_serial();
i32 is_transmit_empty();
void write_char_serial(i8 ch);
void write_string_serial(i8 *str);

#endif
