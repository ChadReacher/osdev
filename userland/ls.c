#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

i32 main(i32 argc, i8 *argv[]) {
	i8 *dirname = malloc(100);
	struct dirent *entry;
	DIR *dirp;
	

	if (argc == 0) {
		return -1;
	} else if (argc > 1) {
		dirname = argv[1];
	} else {
		getcwd(dirname, 100);
	}

	if ((dirp = opendir(dirname)) == NULL) {
		return -1;
	}

	i32 i = 0;
	while ((entry = readdir(dirp)) != NULL) {
		printf("%s\n", entry->name);
	}
	closedir(dirp);
	return 0;
}
