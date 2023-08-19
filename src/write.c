#include "write.h"
#include "syscall.h"

u32 write(i32 fd, i8 *buf, u32 count) {
	u32 ret;

	__asm__ __volatile__ (INT_SYSCALL 
			: "=a"(ret) 
			: "a"(SYSCALL_WRITE), "b"(fd), "c"(buf), "d"(count));

	return ret;
}
