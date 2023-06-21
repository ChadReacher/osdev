#include "util.h"
#include "screen.h"

__attribute__ ((section ("kernel_entry"))) void _start() {
	clear_screen();

	for (u16 i = 0; i < 240; ++i) {
		print_char('V');
	}
	for (u16 j = 1; j < 66; ++j) {
		for (u16 i = 0; i < 240; ++i) {
			u8 ch;
			if (i < 32) {
				ch = 'E';
			} else if (i > 127) {
				ch = 'O';
			} else {
				ch = i;
			}
			if (j != 66 || i != 239) {
				print_char(ch);
			}
		}
	}
	for (u16 i = 0; i < 239; ++i) {
		print_char('S');
	}
	print_char('\r');
	print_char('M');
	print_char('a');
	print_char('n');
	print_char('!');
	print_char('\r');
	print_char('Z');

	for (;;) {}
}
