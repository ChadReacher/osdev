#include "stdio.h"
#include "stdlib.h"
#include "screen.h"
#include "string.h"
#include "serial.h"

void kprintf(i8 *fmt, ...) {
	if (!fmt) {
		return;
	}
	va_list args;
	va_start(args, fmt);
	i8 internal_buf[2048];
	memset(internal_buf, 0, sizeof internal_buf);
	kvsprintf(internal_buf, fmt, args);	
	puts(internal_buf);
	va_end(args);
}

void kvsprintf(i8 *buf, i8 *fmt, va_list args) {
	i8 internal_buf[2048];
	u32 sz;
	i8 *p;
	i8 *temp_s;
	i8 c;

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
				utoa(va_arg(args, u32), internal_buf, 16);
				sz = strlen(internal_buf);
				memcpy(buf, internal_buf, sz);
				buf += sz;
				break;
			case 'c':
				c = va_arg(args, i32);
				if (c) {
					*buf = c;
					++buf;
				}
				break;
			case 's':
				temp_s = va_arg(args, i8*);
				sz = strlen(temp_s);
				memcpy(buf, temp_s, sz);
				buf += sz;
				break;
			case 'p':
				memset(internal_buf, 0, sizeof internal_buf);
				utoa((u32)va_arg(args, void*), internal_buf, 16);
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
}

void putchar(i8 c) {
	screen_print_char(c);
}

void puts(i8 *s) {
	screen_print_string(s);
}
