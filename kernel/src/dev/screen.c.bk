#include <screen.h>
#include <paging.h>
#include <string.h>

static const u32 background_color = 0x00263238; /* Grey color */
static const u32 foreground_color = 0x00FFFFFF; /* White color */

static u16 cursor_x = 0, cursor_y = 0;

void screen_init() {
	u32 fb_size_in_bytes, fb_size_in_pages, i, fb_start;
	cursor_x = 0;
	cursor_y = 0;
	map_page((void *)0xA000, (void *)0xFE000000, PAGING_FLAG_PRESENT);

	/* Identity map framebuffer */
	fb_size_in_bytes = SCREEN_SIZE * 4;
	fb_size_in_pages = fb_size_in_bytes / PAGE_SIZE;
	if (fb_size_in_pages % PAGE_SIZE > 0) ++fb_size_in_pages;
	for (i = 0, fb_start = 0xFD000000; i < fb_size_in_pages; ++i, fb_start += PAGE_SIZE) {
		map_page((void *)fb_start, (void *)fb_start, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
	}

	screen_clear();
}

void screen_clear() {
	u32 i;
	u32 *framebuffer = (u32 *)FRAMEBUFFER_ADDRESS;
	for (i = 0; i < SCREEN_SIZE; ++i) {
		framebuffer[i] = background_color;
	}
	move_cursor();
}

void screen_print_char(i8 ch) {
	u32 *framebuffer;
	u8 *char_glyph;
	u8 line;
	i8 bit;

	if (ch < 0) {
		return;
	}

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

	for (line = 0; line < PIXEL_HEIGHT; ++line) {
		for (bit = PIXEL_WIDTH - 1; bit >= 0; --bit) {
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

	/* PIXEL_HEIGHT - don't copy the last row of chars;  */
	/* SCREEN_HEIGHT % PIXEL_HEIGHT - we have an extra 8 rows, but we can't put there any character, so we need to take into account it */
	u16 line, bit;
	for (line = 0; line < SCREEN_HEIGHT - (PIXEL_HEIGHT + SCREEN_HEIGHT % PIXEL_HEIGHT); ++line) { 
		for (bit = 0; bit < SCREEN_WIDTH; ++bit) {
			*framebuffer = *(framebuffer + PIXEL_HEIGHT * SCREEN_WIDTH);
			++framebuffer;
		}
	}

	for (line = 0; line < PIXEL_HEIGHT; ++line) {
		for (bit = 0; bit < SCREEN_WIDTH; ++bit) {
			*framebuffer++ = background_color; 
		}
	}
}

void move_cursor() {
	u8 ch;
	u32 *framebuffer;
	u8 *char_glyph;
	i8 bit;
	u8 line;

	ch = '|';

	framebuffer = (u32 *)FRAMEBUFFER_ADDRESS;
	framebuffer += cursor_y * PIXEL_HEIGHT * SCREEN_WIDTH + cursor_x * PIXEL_WIDTH;

	char_glyph = (u8 *)(FONT_ADDRESS + ((ch * PIXEL_HEIGHT) - PIXEL_HEIGHT));

	bit = PIXEL_WIDTH - 1;
	for (line = 0; line < PIXEL_HEIGHT; ++line) {
		*framebuffer = (char_glyph[line] & (1 << bit)) ? foreground_color : background_color;
		framebuffer += SCREEN_WIDTH;
	}
}

void remove_cursor() {
	u8 ch;
	u32 *framebuffer;
	u8 *char_glyph;
	i8 bit;
	u8 line;

	ch = ' ';

	framebuffer = (u32 *)FRAMEBUFFER_ADDRESS;
	framebuffer += cursor_y * PIXEL_HEIGHT * SCREEN_WIDTH + cursor_x * PIXEL_WIDTH;

	char_glyph = (u8 *)(FONT_ADDRESS + ((ch * PIXEL_HEIGHT) - PIXEL_HEIGHT));

	bit = PIXEL_WIDTH - 1;
	for (line = 0; line < PIXEL_HEIGHT; ++line) {
		*framebuffer = (char_glyph[line] & (1 << bit)) ? foreground_color : background_color;
		framebuffer += SCREEN_WIDTH;
	}
}

void remove_char() {
	u32 *framebuffer;
	u8 *char_glyph;
	u8 line;
	i8 bit;

	framebuffer = (u32 *)FRAMEBUFFER_ADDRESS;
	framebuffer += cursor_y * PIXEL_HEIGHT * SCREEN_WIDTH + cursor_x * PIXEL_WIDTH;

	char_glyph = (u8 *)(FONT_ADDRESS + ((' ' * PIXEL_HEIGHT) - PIXEL_HEIGHT));
	for (line = 0; line < PIXEL_HEIGHT; ++line) {
		for (bit = PIXEL_WIDTH - 1; bit >= 0; --bit) {
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

