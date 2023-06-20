#include "screen.h"

static const u32 background_color = 0x00000000; // Black color
static const u32 foreground_color = 0x00FFFFFF; // White color

void clear_screen() {
	u32 *framebuffer = *(u32 **)FRAMEBUFFER_ADDRESS;
	for (u32 i = 0; i < SCREEN_SIZE; ++i) {
		*framebuffer++ = background_color;
	}
}

void print_char(u16 x, u16 y, u8 ch) {
	u32 *framebuffer;
	u8 *font_char;

	framebuffer = *(u32 **)FRAMEBUFFER_ADDRESS;
	framebuffer += y * 16 * 1920 + x * 8;

	font_char = (u8 *)(FONT_ADDRESS + ((ch * 16) - 16));

	for (u8 line = 0; line < PIXEL_HEIGHT; ++line) {
		for (i8 bit = PIXEL_WIDTH - 1; bit >= 0; --bit) {
			*framebuffer = (font_char[line] & (1 << bit)) ? foreground_color : background_color;
			++framebuffer;
		}
		framebuffer += (1920 - 8);
	}
}
