#include "util.h"
#include "screen.h"

__attribute__ ((section ("kernel_entry"))) void _start() {
	clear_screen();

	i32 a = 10;

	kprintf("Zero decimal number %d\n", 0);
	kprintf("Positive number %d\n", 1234567890);
	kprintf("Negative number %d \n", -1234567890);
	kprintf("Positive number %i \n", 1234567890);
	kprintf("Negative number %i \n", -1234567890);
	kprintf("Hex number(1) %x\n", 0x1234567890);
	kprintf("Hex number(2) %x\n", 0xABCDEF);
	kprintf("Hex number(3) %x\n", 15);
	kprintf("Zero hex number %x\n", 0);
	kprintf("Test with %% \n");
	kprintf("Default text\n", 1234567890);
	kprintf("Test single character %c\n", 0x41);
	kprintf("Test string %s\n", "some string");
	kprintf("Pointer of variable a %p\n", &a);

	for (;;) {}
}
