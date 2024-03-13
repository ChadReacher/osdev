#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

i32 main(i32 argc, i8 *argv[]) {
	i8 *dirname = malloc(256);
	memset(dirname, 0, 256);
	struct dirent *entry;
	DIR *dirp;
	
	if (argc == 1) {
		getcwd(dirname, 256);
	} else if (argc > 1) {
		memcpy(dirname, argv[1], strlen(argv[1]));
	}

	if ((dirp = opendir(dirname)) == NULL) {
		free(dirname);
		return -1;
	}

	printf("start reading\n");
	while ((entry = readdir(dirp)) != 0) {
		printf("%s\n", entry->name);
	}

	free(dirname);
	closedir(dirp);
	return 0;
}
