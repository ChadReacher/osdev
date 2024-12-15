#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

i32 main(i32 argc, i8 *argv[]) {
	i8 buf[128];
	i32 err;

	memset(buf, 0, sizeof(buf));
	if (argc != 2) {
		printf("%s usage: symlink\n", argv[0]);
		return 1;
	}
	err = readlink(argv[1], buf, sizeof(buf));
	if (err < 0) {
		perror("symlink failed");
	}
	printf("%s\n", buf);
	return 0;
}
