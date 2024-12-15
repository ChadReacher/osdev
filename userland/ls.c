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
		struct stat statbuf;
		i8 *name = malloc(sizeof(char) * (strlen(dirname) + strlen(entry->name) + 2));
		memset(name, 0, strlen(dirname) + strlen(entry->name) + 2);
		memcpy(name, dirname, strlen(dirname));
		name[strlen(dirname)] = '/';
		memcpy(name + strlen(dirname) + 1, entry->name, strlen(entry->name));
		if (lstat(name, &statbuf) != 0) {
			perror("ls: stat failed");
			printf(" %s\n", name);
			free(name);
			continue;
		}
		free(name);
		i8 type[2] = { 0, 0 };
		if (S_ISREG(statbuf.st_mode)) {
			type[0] = '-';
		} else if (S_ISDIR(statbuf.st_mode)) {
			type[0] = 'd';
		} else if (S_ISLNK(statbuf.st_mode)) {
			type[0] = 'l';
		} else if (S_ISCHR(statbuf.st_mode)) {
			type[0] = 'c';
		} else if (S_ISBLK(statbuf.st_mode)) {
			type[0] = 'b';
		}
		printf("[%s] 0x%x %d %d %d(inode) %d(dev) %s\n",
			type,
			statbuf.st_mode,
			statbuf.st_nlink,
			statbuf.st_size,
			statbuf.st_ino,
			statbuf.st_dev,
			entry->name);
	}

	free(dirname);
	closedir(dirp);
	return 0;
}
