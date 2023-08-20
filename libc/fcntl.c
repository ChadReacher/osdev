#include "fcntl.h"

u32 open(i8 *filename, u32 oflags, u32 mode) {
	u32 ret;
	
	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(3), "b"(filename), "c"(oflags), "d"(mode));
	return ret;
}
