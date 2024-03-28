#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#define BUF_SZ (1024)

i32 main(i32 argc, i8 *argv[]) {
	i32 i, j, fd, r, err;
	i8 *buf = malloc(BUF_SZ);

	if (argc == 1) {
		while ((r = read(0, buf, BUF_SZ)) > 0) {
			if ((err = write(1, buf, r)) < 0) {
				perror("cat: write failed");
			}
		}
		return 0;
	}

	for (i = 1; i < argc; ++i) {
		memset(buf, 0, BUF_SZ);
		fd = open(argv[i], O_RDONLY, 0);
		if (fd < 0) {
			printf("could not open: %s\n", argv[i]);
			continue;
		}
		if (read(fd, buf, BUF_SZ) > 0) {
			for (j = 0; j < BUF_SZ; ++j) {
				printf("%c", buf[j]);
			}
		} else {
			printf("could not read: %s\n", argv[i]);
		}
		close(fd);
	}

	return 0;
}
