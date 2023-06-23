#include <stdarg.h>
#include "serial.h"
#include "memory.h"
#include "string.h"
#include "screen.h"
#include "k_stdlib.h"

static const u32 background_color = 0x00000000; // Black color
static const u32 foreground_color = 0x00FFFFFF; // White color

static u16 cursor_x = 0, cursor_y = 0;

void clear_screen() {
	u32 *framebuffer = *(u32 **)FRAMEBUFFER_ADDRESS;
	memset(framebuffer, background_color, SCREEN_SIZE);
}

static void print_char(u8 ch) {
	u32 *framebuffer;
	u8 *font_char;

	framebuffer = *(u32 **)FRAMEBUFFER_ADDRESS;
	framebuffer += cursor_y * PIXEL_HEIGHT * SCREEN_WIDTH + cursor_x * PIXEL_WIDTH;

	if (ch == '\n') {
		cursor_x = 0;
		++cursor_y;
		if (cursor_y >= MAX_CHARS_IN_COL) {
			scroll_up();
			cursor_y = MAX_CHARS_IN_COL - 1;
		}
		return;
	} else if (ch == '\r') {
		cursor_x = 0;
		return;
	}

	font_char = (u8 *)(FONT_ADDRESS + ((ch * PIXEL_HEIGHT) - PIXEL_HEIGHT));

	for (u8 line = 0; line < PIXEL_HEIGHT; ++line) {
		for (i8 bit = PIXEL_WIDTH - 1; bit >= 0; --bit) {
			*framebuffer = (font_char[line] & (1 << bit)) ? foreground_color : background_color;
			++framebuffer;
		}
		framebuffer += (SCREEN_WIDTH - PIXEL_WIDTH);
	}
	++cursor_x;

	if (cursor_x >= MAX_CHARS_IN_ROW) {
		cursor_x = 0;
		++cursor_y;
		if (cursor_y >= MAX_CHARS_IN_COL) {
			scroll_up();
			cursor_x = 0;
			cursor_y = MAX_CHARS_IN_COL - 1;
		}
	}
}

static void print_string(i8 *string) {
	while (*string) {
		print_char(*string++);
	}
}

static void scroll_up() {
	u32 *framebuffer = *(u32 **)FRAMEBUFFER_ADDRESS;

	// PIXEL_HEIGHT - don't copy the last row of chars; 
	// SCREEN_HEIGHT % PIXEL_HEIGHT - we have an extra 8 rows, but we can't put there any character, so we need to take into account it
	for (u16 line = 0; line < SCREEN_HEIGHT - (PIXEL_HEIGHT + SCREEN_HEIGHT % PIXEL_HEIGHT); ++line) { 
		for (u16 bit = 0; bit < SCREEN_WIDTH; ++bit) {
			*framebuffer = *(framebuffer + PIXEL_HEIGHT * SCREEN_WIDTH);
			++framebuffer;
		}
	}

	for (u16 line = 0; line < PIXEL_HEIGHT; ++line) {
		for (u16 bit = 0; bit < SCREEN_WIDTH; ++bit) {
			*framebuffer++ = background_color; 
		}
	}
}

void kprintf(u8 *fmt, ...) {
	va_list args;

	va_start(args, fmt);

	u8 internal_buf[1024];
	memset(internal_buf, 0, sizeof internal_buf);

	kvsprintf(internal_buf, fmt, args);

	print_string(internal_buf);

	size_t sz = strlen(internal_buf);
	internal_buf[sz] = '\r';
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
