#include "fcntl.h"
#include "unistd.h"
#include "errno.h"

syscall3(i32, open, const i8 *, filename, u32, oflags, u32, mode)

u32 creat(i8 *path, u32 mode) {
	return open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
}
