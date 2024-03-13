#include "dirent.h"
#include "fcntl.h"
#include "sys/stat.h"
#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"

DIR *opendir(const i8 *name) {
	i32 fd;
	struct stat st;
	DIR *dirp;

	fd = open(name, O_RDONLY, 0);
	if (fd < 0) {
		printf("opendir: bad open fd: %d\n", fd);
		return NULL;
	}

	if (fstat(fd, &st) < 0) {
		printf("bad fstat\n");
		close(fd);
		return NULL;
	}

	if ((st.st_mode & S_IFDIR) != S_IFDIR) {
		printf("it's not a dir\n");
		close(fd);
		return NULL;
	}

	dirp = malloc(sizeof(DIR));
	if (dirp == NULL) {
		printf("dirp = NULL\n");
		close(fd);
		return NULL;
	}

	dirp->fd = fd;
	memset(&dirp->dent, 0, sizeof(dirp->dent));
	return dirp;
}


struct dirent *readdir(DIR *dirp) {
	struct dirent *ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_readdir), "b"(dirp));

	return ret;
}

i32 closedir(DIR *dirp) {
	i32 ret;

	if (dirp == NULL || dirp->fd < 0) {
		return -1;
	}

	ret = close(dirp->fd);
	free(dirp);
	return ret;
}

