#include <panic.h>
#include <stdio.h>
#include <stdarg.h>
#include <screen.h>

void kernel_panic(i8 *fmt, ...) {
	i8 buf[1024];
	va_list arg;

	va_start(arg, fmt);

	kvsprintf(buf, fmt, arg);
	kprintf(buf);
	kprintf("\nSystem halted!\n");

	__asm__ __volatile__ ("cli");

	while (1);
}
