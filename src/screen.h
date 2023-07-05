#ifndef SCREEN_H
#define SCREEN_H

#include "types.h"

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)
#define PIXEL_WIDTH 8
#define PIXEL_HEIGHT 16
#define MAX_CHARS_IN_ROW (SCREEN_WIDTH / PIXEL_WIDTH)
#define MAX_CHARS_IN_COL (SCREEN_HEIGHT / PIXEL_HEIGHT)

#define FRAMEBUFFER_ADDRESS 0x9028
#define FONT_ADDRESS 0xA000

void screen_clear();
void print_char(i8 ch);
void scroll_up();
void print_string(i8 *string);
void screen_backspace();
void remove_char();

void remove_cursor();
void move_cursor();
void move_cursor_left();
void move_cursor_right();
void move_cursor_up();
void move_cursor_down();

#endif
