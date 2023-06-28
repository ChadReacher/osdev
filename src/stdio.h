#ifndef STDIO_H
#define STDIO_H

#include "stdarg.h"
#include "types.h"

void kprintf(i8 *fmt, ...);
void kvsprintf(i8 *buf, i8 *fmt, va_list args);

#endif
