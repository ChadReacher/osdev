#include "types.h"
#include "syscall.h"

i32 lseek(i32 fd, i32 offset, i32 whence) {
	u32 ret;

	__asm__ __volatile__ (INT_SYSCALL 
			: "=a"(ret) 
			: "a"(SYSCALL_LSEEK), "b"(fd), "c"(offset), "d"(whence));

	return ret;
}
