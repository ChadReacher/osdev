#include "screen.h"

static const u32 background_color = 0x00000000; // Black color
static const u32 foreground_color = 0x00FFFFFF; // White color

static u16 cursor_x = 0, cursor_y = 0;

void clear_screen() {
	u32 *framebuffer = *(u32 **)FRAMEBUFFER_ADDRESS;
	for (u32 i = 0; i < SCREEN_SIZE; ++i) {
		*framebuffer++ = background_color;
	}
}

void print_char(u8 ch) {
	u32 *framebuffer;
	u8 *font_char;

	framebuffer = *(u32 **)FRAMEBUFFER_ADDRESS;
	framebuffer += cursor_y * PIXEL_HEIGHT * SCREEN_WIDTH + cursor_x * PIXEL_WIDTH;

	if (ch == '\n') {
		cursor_x = 0;
		++cursor_y;
		if (cursor_y >= 67) {
			scroll_up();
			cursor_y = 66;
		}
		return;
	} else if (ch == '\r') {
		cursor_x = 0;
		return;
	}

	font_char = (u8 *)(FONT_ADDRESS + ((ch * 16) - 16));

	for (u8 line = 0; line < PIXEL_HEIGHT; ++line) {
		for (i8 bit = PIXEL_WIDTH - 1; bit >= 0; --bit) {
			*framebuffer = (font_char[line] & (1 << bit)) ? foreground_color : background_color;
			++framebuffer;
		}
		framebuffer += (1920 - 8);
	}
	++cursor_x;

	if (cursor_x >= 240) {
		cursor_x = 0;
		++cursor_y;
		if (cursor_y >= 67) {
			scroll_up();
			cursor_x = 0;
			cursor_y = 66;
		}
	}
}

void scroll_up() {
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
