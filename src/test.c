#include "test.h"
#include "syscall.h"

void test(const i8 *s) {
	__asm__ __volatile__ (INT_SYSCALL : /* no output */ : "a"(SYSCALL_TEST), "b"(s));
}
