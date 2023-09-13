#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

i32 main(i32 argc, i8 *argv[]) {
	i8 *dirname = malloc(256);
	struct dirent *entry;
	DIR *dirp;
	

	if (argc == 0) {
		free(dirname);
		return -1;
	} else {
		getcwd(dirname, 256);
	}

	if ((dirp = opendir(dirname)) == NULL) {
		free(dirname);
		return -1;
	}

	while ((entry = readdir(dirp)) != NULL) {
		printf("%s\n", entry->name);
	}

	free(dirname);
	closedir(dirp);
	return 0;
}
