#include "dirent.h"
#include "fcntl.h"
#include "sys/stat.h"
#include "stdlib.h"
#include "unistd.h"
#include "stdio.h"
#include "string.h"

DIR *opendir(const i8 *name) {
	i32 fd;
	struct stat st;
	DIR *dirp;

	fd = open(name, O_RDONLY, 0);
	if (fd < 0) {
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
		close(fd);
		return NULL;
	}

	dirp->fd = fd;
	memset(&dirp->dent, 0, sizeof(dirp->dent));
	return dirp;
}

i32 closedir(DIR *dirp) {
	if (dirp == NULL || dirp->fd < 0) {
		return -1;
	}

	i32 ret = close(dirp->fd);
	free(dirp);
	return ret;
}

struct dirent *readdir(DIR *dirp) {
	i32 ret = read(dirp->fd, &dirp->dent, sizeof(struct dirent));
	if (!ret) {
		return NULL;
	}
	return &dirp->dent;
}
