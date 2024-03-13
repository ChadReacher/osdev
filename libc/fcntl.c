#include "fcntl.h"
#include "unistd.h"

u32 open(const i8 *filename, u32 oflags, u32 mode) {
	u32 ret;
	
	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_open), "b"(filename), "c"(oflags), "d"(mode));
	return ret;
}

u32 creat(i8 *path, u32 mode) {
	return open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
}
