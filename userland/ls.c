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
		printf("%s\n", entry->name);
	}

	free(dirname);
	closedir(dirp);
	return 0;
}
