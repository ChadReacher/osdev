#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

i32 main(i32 argc, i8 *argv[]) {
	if (argc < 2) {
		printf("\t %s usage: file\n", argv[0]);
	}

	i8 buf[1024];
	for (i32 i = 1; i < argc; ++i) {
		i32 fd = open(argv[i], O_RDONLY, 0);

		if (read(fd, buf, 1024) > 0) {
			printf("%s", buf);
		} else {
			printf("could not read: %s\n", argv[i]);
		}

		close(fd);
	}

	return 0;
}
