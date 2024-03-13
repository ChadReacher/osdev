#include <panic.h>
#include <stdio.h>
#include <stdarg.h>
#include <screen.h>

void debug(i8 *fmt, ...) {
	(void)fmt;
	/*serial_printf("DEBUG: %s:%d:%s(): " format "\r\n", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__); */
}

void panic(i8 *fmt, ...) {
	i8 buf[1024];
	va_list arg;

	va_start(arg, fmt);

	kvsprintf(buf, fmt, arg);
	kprintf(buf);
	kprintf("\nSystem halted!\n");

	__asm__ __volatile__ ("cli; hlt");

	while (1);
}
