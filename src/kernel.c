#include "util.h"

void dummy_test_entrypoint() {
}

__attribute__ ((section ("kernel_entry"))) void _start() {
	u32 **framebuffer = (u32 **)0x5028;
	for (u32 i = 0; i < 1920 * 1080; ++i) {
		*(*framebuffer + i) = 0x000000FF;
	}

	for (;;) {}
}
