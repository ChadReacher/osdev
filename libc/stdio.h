#ifndef STDIO_H
#define STDIO_H

#include "stdarg.h"
#include "types.h"

#define stdin 0
#define stdout 1
#define stderr 2

void printf(const i8 *fmt, ...);
void vsprintf(i8 *buf, const i8 *fmt, va_list args);

#endif
