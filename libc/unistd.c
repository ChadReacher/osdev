#include "unistd.h"

void test(const i8 *s) {
	__asm__ __volatile__ ("int $0x80" : /* no output */ : "a"(0), "b"(s));
}

u32 read(i32 fd, const void *buf, u32 count) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(1), "b"(fd), "c"(buf), "d"(count));

	return ret;
}

u32 write(i32 fd, const void *buf, u32 count) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(2), "b"(fd), "c"(buf), "d"(count));

	return ret;
}

i32 close(i32 fd) {
	i32 ret;
	__asm__ __volatile__ ("int $0x80" : "=a"(ret) : "a"(4), "b"(fd));
	return ret;
}

i32 lseek(i32 fd, i32 offset, i32 whence) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(5), "b"(fd), "c"(offset), "d"(whence));

	return ret;
}

i32 unlink(const i8 *pathname) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(6), "b"(pathname));

	return ret;
}

void yield() {
	__asm__ __volatile__ ("int $0x80" : : "a"(7));
}

i32 exec(const i8 *pathname) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(8), "b"(pathname));

	return ret;
}

i32 fork() {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(9));

	return ret;
}

void exit() {
	__asm__ __volatile__ ("int $0x80" : : "a"(10));
}
