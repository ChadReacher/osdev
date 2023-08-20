#include "close.h"
#include "syscall.h"

i32 close(i32 fd) {
	i32 ret;
	__asm__ __volatile__ (INT_SYSCALL : "=a"(ret) : "a"(SYSCALL_CLOSE), "b"(fd));
	return ret;
}
