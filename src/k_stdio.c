#include "k_stdio.h"
#include "k_stdlib.h"
#include <stdarg.h>
#include "screen.h"
#include "memory.h"
#include "string.h"
#include "serial.h"

void kprintf(u8 *fmt, ...) {
	va_list args;

	va_start(args, fmt);

	u8 internal_buf[1024];
	memset(internal_buf, 0, sizeof internal_buf);

	kvsprintf(internal_buf, fmt, args);

	print_string(internal_buf);

	size_t sz = strlen(internal_buf);
	if (internal_buf[sz - 1] == '\n') {
		internal_buf[sz] = '\r';
	}
	write_string_serial(internal_buf);
}

void kvsprintf(u8 *buf, u8 *fmt, va_list args) {
	u8 internal_buf[512];
	size_t sz;
	u8 *p;
	i8 *temp_s;

	for (p = fmt; *p; ++p) {
		if (*p != '%') {
			*buf = *p;
			++buf;
			continue;
		}
		switch (*++p) {
			case 'd':
			case 'i':
				memset(internal_buf, 0, sizeof internal_buf);
				itoa(va_arg(args, i32), internal_buf, 10);
				sz = strlen(internal_buf);
				memcpy(buf, internal_buf, sz);
				buf += sz;
				break;
			case 'x':
				memset(internal_buf, 0, sizeof internal_buf);
				itoa(va_arg(args, i32), internal_buf, 16);
				sz = strlen(internal_buf);
				memcpy(buf, internal_buf, sz);
				buf += sz;
				break;
			case 'c':
				*buf = (i8) va_arg(args, i32);
				++buf;
				break;
			case 's':
				temp_s = va_arg(args, i8*);
				sz = strlen(temp_s);
				memcpy(buf, temp_s, sz);
				buf += sz;
				break;
			case 'p':
				memset(internal_buf, 0, sizeof internal_buf);
				itoa((u32)va_arg(args, void*), internal_buf, 16);
				sz = strlen(internal_buf);
				memcpy(buf, internal_buf, sz);
				buf += sz;
				break;
			case '%':
				*buf = *p;
				++buf;
				break;
			default:
				break;
		}
	}
	va_end(args);
}
