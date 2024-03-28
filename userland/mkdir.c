#include <stdio.h>
#include <unistd.h>

i32 main(i32 argc, i8 *argv[]) {
	i32 err;
	if (argc != 2) {
		printf("usage: mkdir absolute-path\n");
		return 1;
	}
	err = mkdir(argv[1], 777);
	if (err) {
		perror("mkdir failed");
	}
	return 0;
}
