#include "unlink.h"
#include "syscall.h"

i32 unlink(i8 *pathname) {
	i32 ret;

	__asm__ __volatile__ (INT_SYSCALL 
			: "=a"(ret) 
			: "a"(SYSCALL_UNLINK), "b"(pathname));

	return ret;
}
