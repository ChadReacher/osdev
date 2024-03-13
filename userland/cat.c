#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUF_SZ 1024

i32 main(i32 argc, i8 *argv[]) {
	i8 buf[BUF_SZ] = {0};

	if (argc == 1) {
		while (true) {
			u32 i = 0;
			u8 c;
			do {
				c = getchar();
				putchar(c);
				buf[i++] = c;
			} while (c != '\n');
			printf(buf);
			memset(buf, 0, BUF_SZ);
		};
	}

	for (i32 i = 1; i < argc; ++i) {
		memset(buf, 0, BUF_SZ);
		i32 fd = open(argv[i], O_RDONLY, 0);
		if (fd < 0) {
			printf("could not open: %s\n", argv[i]);
			continue;
		}

		if (read(fd, buf, BUF_SZ) > 0) {
			for (int i = 0; i < 1024; ++i) {
				printf("%c", buf[i]);
			}
		} else {
			printf("could not read: %s\n", argv[i]);
		}

		close(fd);
	}

	return 0;
}
