#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

i32 main(i32 argc, i8 *argv[]) {
	i32 err;
	if (argc != 3) {
		printf("%s usage: file symlink_name\n", argv[0]);
		return 1;
	}
	err = symlink(argv[1], argv[2]);
	if (err) {
		perror("symlink failed");
	}
	return 0;
}
