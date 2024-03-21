#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

i32 main(i32 argc, i8 *argv[]) {
	DIR *dirp;
	struct dirent *entry;
	i8 *dirname;
	struct stat statbuf;
	dirname = malloc(256);

	memset(dirname, 0, 256);
	
	if (argc == 1) {
		getcwd(dirname, 256);
	} else if (argc > 1) {
		memcpy(dirname, argv[1], strlen(argv[1]));
	}

	if ((dirp = opendir(dirname)) == NULL) {
		free(dirname);
		return -1;
	}


	while ((entry = readdir(dirp)) != 0) {
		printf("name: %s ", entry->name);
		stat(entry->name, &statbuf);
		printf("dev: %d ", statbuf.st_dev);
		printf("inode: %d ", statbuf.st_ino);
		printf("mode: %d ", statbuf.st_mode);
		printf("link count: %d ", statbuf.st_nlink);
		printf("uid: %d ", statbuf.st_uid);
		printf("gid: %d ", statbuf.st_gid);
		printf("size: %d ", statbuf.st_size);
		printf("access time: %d ", statbuf.st_atime);
		printf("modified time: %d ", statbuf.st_mtime);
		printf("changed time: %d ", statbuf.st_ctime);
		printf("blksize: %d ", statbuf.st_blksize);
		printf("blocks(512): %d\n", statbuf.st_blocks);
	}

	free(dirname);
	closedir(dirp);
	return 0;
}
