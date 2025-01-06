#include <panic.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <serial.h>

void debug(i8 *fmt, ...) {
	i8 buf[512] = {0};
	va_list args;

	va_start(args, fmt);
	kvsprintf(buf, fmt, args);
        va_end(args);
	write_string_serial(buf);
}

void panic(i8 *fmt, ...) {
	i8 buf[512] = {0};
	va_list arg;

	va_start(arg, fmt);
	kvsprintf(buf, fmt, arg);
        va_end(args);
	kprintf(buf);
	kprintf("\nSystem halted!\n");
	__asm__ volatile ("cli; hlt");
	while (1);
}
