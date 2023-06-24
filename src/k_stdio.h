#ifndef K_STDIO_H
#define K_STDIO_H

#include <stdarg.h>
#include "types.h"

void kprintf(u8 *fmt, ...);
void kvsprintf(u8 *buf, u8 *fmt, va_list args);

#endif
