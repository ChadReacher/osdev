#include <console.h>
#include <paging.h>
#include <tty.h>
#include <string.h>
#include <port.h>

enum state {
	NORMAL,
	ESCAPE,
	SQUARE,
	GETPARAMS,
	GOTPARAMS
};

#define NPARAMS 16
#define ROWS 25
#define COLUMNS 80
#define SCREEN_START 0xFE000000
#define SCREEN_END	0xFE008000
#define ORIGIN SCREEN_START
#define END (ORIGIN + ROWS * COLUMNS * 2)

static u32 pos;
static u32 state;
static u32 x, y;

static u32 nparams;
static u32 params[NPARAMS];

static void scroll_down() {
	u8 *x;
	i32 i;
	for (i = 1; i < ROWS; ++i) {
		memcpy((void *)(SCREEN_START + (COLUMNS * 2) * (i - 1)),
				(void *)(SCREEN_START + (COLUMNS * 2) * i), COLUMNS * 2);
	}
	x = (u8 *)(SCREEN_START + (COLUMNS * 2) * (ROWS - 1));
	for (i = 0; i < COLUMNS * 2; ++i) {
		*x = 0;
		x += 2;
	}
}

static void line_feed() {
	++y;
	pos += (COLUMNS * 2);
}

static void carriage_return() {
	pos -= (x * 2);
	x = 0;
}

static void csi_j(u32 param) {
	u16 *start;
	u32 count, i;
	if (param == 0) {
		start = (u16 *)pos;
		count = (SCREEN_END - pos) / 2;
	} else if (param == 1) {
		start = (u16 *)SCREEN_START;
		count = (pos - SCREEN_START) / 2;
	} else if (param == 2) {
		start = (u16 *)SCREEN_START;
		count = COLUMNS * ROWS;
	} else {
		return;
	}
	for (i = 0; i < count; ++i, ++start) {
		*start = ((0x07 << 8) | ' ');
	}
}

static void csi_k(u32 param) {
	u16 *start;
	u32 count, i;
	if (param == 0) {
		if (x >= COLUMNS) {
			return;
		}
		start = (u16 *)pos;
		count = COLUMNS - x;
	} else if (param == 1) {
		start = (u16 *)(pos - (x * 2));
		count = (x < COLUMNS?x:COLUMNS);
	} else if (param == 2) {
		start = (u16 *)(pos - (x * 2));
		count = COLUMNS;
	} else {
		return;
	}
	for (i = 0; i < count; ++i, ++start) {
		*start = ((0x07 << 8) | ' ');
	}
}

static void goto_xy(u32 new_x, u32 new_y) {
	if (new_x >= COLUMNS || new_y >= ROWS) {
		return;
	}
	x = new_x;
	y = new_y;
	pos = SCREEN_START + ((y * COLUMNS + x) * 2);
}

static void update_cursor() {
	u16 curpos = (y * COLUMNS) + x;
	port_outb(0x3D4, 0xE);
	port_outb(0x3D5, ((curpos >> 8) & 0xFF));
	port_outb(0x3D4, 0xF);
	port_outb(0x3D5, (curpos & 0xFF));
}

void console_write(struct tty_struct *tty) {
	i32 count;
	i8 c;

	count = CHARS(tty->output);
	while (count--) {
		if (tty->stopped) {
			break;
		}
		c = ttyq_getchar(&tty->output);
		switch (state) {
			case NORMAL:
				if (c > 31 && c < 127) {
					*(u16 *)pos = (c | (0x07 << 8));
					pos += 2;
					++x;
				} else if (c == 9) {
					c = 8 - (x & 7);
					x += c;
					pos += (c * 2);
					c = 9;
				} else if (c == 10 || c == 11 || c == 12) {
					line_feed();
				} else if (c == 27) {
					state = ESCAPE;
				} else if (c == 13) {
					carriage_return();
				} else if (c == 127) {
					if (x > 0) {
						--x;
						pos -= 2;
						*(u16 *)pos = (' ' | (0x07 << 8));
					}
				}
				if (x >= COLUMNS) {
					x -= COLUMNS;
					pos -= (COLUMNS * 2);
					pos += (COLUMNS * 2);
					++y;
				}
				if (y >= ROWS) {
					pos -= (COLUMNS * 2);
					y = ROWS - 1;
					scroll_down();
				}
				break;
		case ESCAPE:
				state = NORMAL;
				if (c == '[') {
					state = SQUARE;
				}
				break;
		case SQUARE:
				for (nparams = 0; nparams < NPARAMS; ++nparams) {
					params[nparams] = 0;
				}
				nparams = 0;
				state = GETPARAMS;
				/* fall through */
		case GETPARAMS:
				if (c == ';' && nparams < NPARAMS - 1) {
					++nparams;
					break;
				} else if (c >= '0' && c <= '9') {
					params[nparams] = 10 * params[nparams] + c - '0';
					break;
				} else {
					state = GOTPARAMS;
				}
				/* fall through */
		case GOTPARAMS:
				state = NORMAL;
				switch (c) {
					case 'A':
						if (!params[0]) {
							++params[0];
						}
						goto_xy(x, y - params[0]);
						break;
					case 'B':
						if (!params[0]) {
							++params[0];
						}
						goto_xy(x, y + params[0]);
						break;
					case 'C':
						if (!params[0]) {
							++params[0];
						}
						goto_xy(x + params[0], y);
						break;
					case 'D':
						if (!params[0]) {
							++params[0];
						}
						goto_xy(x - params[0], y);
						break;
					case 'E':
						if (!params[0]) {
							++params[0];
						}
						goto_xy(0, y + params[0]);
						break;
					case 'F':
						if (!params[0]) {
							++params[0];
						}
						goto_xy(0, y - params[0]);
						break;
					case 'G':
						if (!params[0]) {
							++params[0];
						}
						goto_xy(params[0], y);
						break;
					case 'H':
						if (params[0]) {
							--params[0];
						}
						if (params[1]) {
							--params[0];
						}
						goto_xy(params[0], params[1]);
						break;
					case 'J':
						csi_j(params[0]);
						break;
					case 'K':
						csi_k(params[0]);
						break;

				}
				break;
		}
	}
	update_cursor();
}

void console_init() {
	u32 i, phys_addr, virt_addr;

	phys_addr = 0xB8000;
	virt_addr = SCREEN_START;
	for (i = 0; i < 0x8000; i += 0x1000) {
		map_page((void *)(phys_addr + i), (void *)(virt_addr + i),
				PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
	}
	x = y = 0;
	pos = SCREEN_START;
	state = NORMAL;
	update_cursor();
}
