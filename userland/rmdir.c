#include <stdio.h>
#include <unistd.h>

i32 main(i32 argc, i8 *argv[]) {
	i32 err;
	if (argc != 2) {
		printf("usage: rmdir absolute-path");
		return 1;
	}
	err = rmdir(argv[1]);
	if (err) {
		perror("rmdir failed");
	}
	return 0;
}
