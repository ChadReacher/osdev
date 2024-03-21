#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

i32 main(i32 argc, i8 *argv[]) {
	i32 err;
	if (argc != 3) {
		printf("need two args\n");
		return 1;
	}
	err = link(argv[1], argv[2]);
	if (err) {
		perror("link failed");
	}
	return 0;
}
