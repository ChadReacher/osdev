#ifndef DEBUG_H
#define DEBUG_H

#include "serial.h"

#define DEBUG(format, ...) serial_printf(\
							"DEBUG: %s:%d:%s(): " format, __FILE__, __LINE__, __func__, __VA_ARGS__)

#endif
