#ifndef SCREEN_H
#define SCREEN_H

#include <stdarg.h>
#include "types.h"

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

#define PIXEL_WIDTH 8
#define PIXEL_HEIGHT 16

#define MAX_CHARS_IN_ROW (SCREEN_WIDTH / PIXEL_WIDTH)
#define MAX_CHARS_IN_COL (SCREEN_HEIGHT / PIXEL_HEIGHT)

#define FRAMEBUFFER_ADDRESS 0x9028
#define FONT_ADDRESS 0x6000

void clear_screen();
void kprintf(u8 *fmt, ...);
void kvsprintf(u8 *buf, u8 *fmt, va_list args);
static void print_char(u8 ch);
static void scroll_up();
static void print_string(i8 *string);

#endif
