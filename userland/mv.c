#include <stdio.h>
#include <unistd.h>

i32 main(i32 argc, i8 *argv[]) {
	i32 err;
	if (argc != 3) {
		printf("usage: mv absolute-path absolute-path");
		return 1;
	}
	err = rename(argv[1], argv[2]);
	if (err) {
		perror("mv failed");
	}
	return 0;
}
