#ifndef DEBUG_H
#define DEBUG_H

#include "serial.h"

#define __DEBUG(format, ...) serial_printf(\
							"DEBUG: %s:%d:%s(): " format, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define DEBUG(...) __DEBUG(__VA_ARGS__, "")

#endif
