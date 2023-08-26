#include <screen.h>
#include <paging.h>
#include <string.h>

static const u32 background_color = 0x00263238; // Grey color
static const u32 foreground_color = 0x00FFFFFF; // White color

static u16 cursor_x = 0, cursor_y = 0;

void screen_clear() {
	u32 *framebuffer = (u32 *)FRAMEBUFFER_ADDRESS;
	for (u32 i = 0; i < SCREEN_SIZE; ++i) {
		framebuffer[i] = background_color;
	}
	cursor_x = 0;
	cursor_y = 0;
	map_page(0xA000, 0xFE000000);
	move_cursor();
}

void screen_print_char(i8 ch) {
	u32 *framebuffer;
	u8 *char_glyph;

	framebuffer = (u32 *)FRAMEBUFFER_ADDRESS;
	framebuffer += cursor_y * PIXEL_HEIGHT * SCREEN_WIDTH + cursor_x * PIXEL_WIDTH;

	if (ch == '\n') {
		remove_cursor();

		cursor_x = 0;
		++cursor_y;
		if (cursor_y >= MAX_CHARS_IN_COL) {
			screen_scroll_up();
			cursor_y = MAX_CHARS_IN_COL - 1;
		}
		move_cursor();
		return;
	} else if (ch == '\r') {
		remove_cursor();

		cursor_x = 0;
		move_cursor();
		return;
	} else if (ch == '\b') {
		remove_cursor(); 
		if (cursor_x == 0) {
			if (cursor_y != 0) {
				cursor_x = MAX_CHARS_IN_ROW - 1;
				--cursor_y;
			}
		} else {
			--cursor_x;
		}
		remove_char();
		move_cursor();

		return;
	}

	char_glyph = (u8 *)(FONT_ADDRESS + ((ch * PIXEL_HEIGHT) - PIXEL_HEIGHT));

	for (u8 line = 0; line < PIXEL_HEIGHT; ++line) {
		for (i8 bit = PIXEL_WIDTH - 1; bit >= 0; --bit) {
			*framebuffer = (char_glyph[line] & (1 << bit)) ? foreground_color : background_color;
			++framebuffer;
		}
		framebuffer += (SCREEN_WIDTH - PIXEL_WIDTH);
	}
	++cursor_x;

	if (cursor_x >= MAX_CHARS_IN_ROW) {
		cursor_x = 0;
		++cursor_y;
		if (cursor_y >= MAX_CHARS_IN_COL) {
			screen_scroll_up();
			cursor_x = 0;
			cursor_y = MAX_CHARS_IN_COL - 1;
		}
	}
	move_cursor();
}

void screen_print_string(i8 *string) {
	while (*string) {
		screen_print_char(*string++);
	}
}

void screen_scroll_up() {
	u32 *framebuffer = (u32 *)FRAMEBUFFER_ADDRESS;

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

void move_cursor() {
	u8 ch;
	u32 *framebuffer;
	u8 *char_glyph;

	ch = '|';

	framebuffer = (u32 *)FRAMEBUFFER_ADDRESS;
	framebuffer += cursor_y * PIXEL_HEIGHT * SCREEN_WIDTH + cursor_x * PIXEL_WIDTH;

	char_glyph = (u8 *)(FONT_ADDRESS + ((ch * PIXEL_HEIGHT) - PIXEL_HEIGHT));

	i8 bit = PIXEL_WIDTH - 1;
	for (u8 line = 0; line < PIXEL_HEIGHT; ++line) {
		*framebuffer = (char_glyph[line] & (1 << bit)) ? foreground_color : background_color;
		framebuffer += SCREEN_WIDTH;
	}
}

void remove_cursor() {
	u8 ch;
	u32 *framebuffer;
	u8 *char_glyph;

	ch = ' ';

	framebuffer = (u32 *)FRAMEBUFFER_ADDRESS;
	framebuffer += cursor_y * PIXEL_HEIGHT * SCREEN_WIDTH + cursor_x * PIXEL_WIDTH;

	char_glyph = (u8 *)(FONT_ADDRESS + ((ch * PIXEL_HEIGHT) - PIXEL_HEIGHT));

	i8 bit = PIXEL_WIDTH - 1;
	for (u8 line = 0; line < PIXEL_HEIGHT; ++line) {
		*framebuffer = (char_glyph[line] & (1 << bit)) ? foreground_color : background_color;
		framebuffer += SCREEN_WIDTH;
	}
}

void remove_char() {
	u32 *framebuffer;
	u8 *char_glyph;

	framebuffer = (u32 *)FRAMEBUFFER_ADDRESS;
	framebuffer += cursor_y * PIXEL_HEIGHT * SCREEN_WIDTH + cursor_x * PIXEL_WIDTH;

	char_glyph = (u8 *)(FONT_ADDRESS + ((' ' * PIXEL_HEIGHT) - PIXEL_HEIGHT));
	for (u8 line = 0; line < PIXEL_HEIGHT; ++line) {
		for (i8 bit = PIXEL_WIDTH - 1; bit >= 0; --bit) {
			*framebuffer = (char_glyph[line] & (1 << bit)) ? foreground_color : background_color;
			++framebuffer;
		}
		framebuffer += (SCREEN_WIDTH - PIXEL_WIDTH);
	}
}

void move_cursor_left() {
	remove_cursor();
	if (cursor_x == 0) {
		if (cursor_y != 0) {
			cursor_x = MAX_CHARS_IN_ROW - 1;
			--cursor_y;
		}
	} else {
		--cursor_x;
	}
	
	move_cursor();
}

void move_cursor_right() {
	remove_cursor();
	++cursor_x;
	if (cursor_x >= MAX_CHARS_IN_ROW) {
		cursor_x = 0;
		if (cursor_y != MAX_CHARS_IN_COL - 1) {
			++cursor_y;
		}
	}
	move_cursor();
}

void move_cursor_up() {
	remove_cursor();
	if (cursor_y != 0) {
		--cursor_y;
	}
	move_cursor();
}

void move_cursor_down() {
	remove_cursor();
	if (cursor_y != MAX_CHARS_IN_COL - 1) {
		++cursor_y;
	}
	move_cursor();
}
