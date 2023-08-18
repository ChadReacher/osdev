#include "fd.h"
#include "vfs.h"

file fds[NB_DESCRIPTORS] = {0};

i32 fd_get() {
	for (u32 i = 3; i < NB_DESCRIPTORS; ++i) {
		if (!fds[i].used) {
			fds[i].used = true;
			return i;
		}
	}
	return -1;
}

void fd_release(i32 id) {
	if (id >= NB_DESCRIPTORS) {
		return;
	}
	memset(&fds[id], 0, sizeof(file));
}
