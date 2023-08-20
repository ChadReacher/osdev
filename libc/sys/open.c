#include "open.h"
#include "syscall.h"

u32 open(i8 *filename, u32 oflags, u32 mode) {
	u32 ret;
	
	__asm__ __volatile__ (INT_SYSCALL 
			: "=a"(ret) 
			: "a"(SYSCALL_OPEN), "b"(filename), "c"(oflags), "d"(mode));
	return ret;
}
