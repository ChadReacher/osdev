#ifndef SCREEN_H
#define SCREEN_H

#include "util.h"

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

#define PIXEL_WIDTH 8
#define PIXEL_HEIGHT 16

#define FRAMEBUFFER_ADDRESS 0x9028
#define FONT_ADDRESS 0x6000

void print_char(u16 x, u16 y, u8 ch);
void clear_screen();

#endif
