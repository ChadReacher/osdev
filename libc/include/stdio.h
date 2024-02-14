#ifndef STDIO_H
#define STDIO_H

#include "stdarg.h"
#include "sys/types.h"

#define stdin 0
#define stdout 1
#define stderr 2

i32 putchar(i32 c);
i32 getchar();

void printf(const i8 *fmt, ...);
void vsprintf(i8 *buf, const i8 *fmt, va_list args);

#endif
