#include "util.h"
#include "screen.h"

__attribute__ ((section ("kernel_entry"))) void _start() {
	clear_screen();

	for (u16 i = 0; i < 240; ++i) {
		u8 ch;
		if (i < 32) {
			ch = '^';
		} else if (i > 127) {
			ch = 'E';
		} else {
			ch = i;
		}
		print_char(i, 0, ch);
	}

	for (u16 i = 0; i < 67; ++i) {
		u8 ch;
		if (i < 32) {
			ch = '^';
		} else if (i > 127) {
			ch = 'E';
		} else {
			ch = i;
		}
		print_char(120, i, ch);
	}

	for (;;) {}
}
