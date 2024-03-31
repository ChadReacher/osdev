#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define BUFSZ 512

char buf[BUFSZ];

void wc(int fd, char *name) {
	int i, n, inword;
	int bytes, words, lines;

	bytes = words = lines = 0;
	inword = 0;
	while ((n = read(fd, buf, BUFSZ)) > 0) {
		for (i = 0; i < n; ++i) {
			++bytes;
			if (buf[i] == '\n') {
				++lines;
			}
			if (strchr(" \r\t\n", buf[i])) {
				inword = 0;
			} else if (!inword) {
				++words;
				inword = 1;
			}
		}
	}
	printf("\t%d\t%d\t%d\t%s\n", lines, words, bytes, name);
}

int main(int argc, char **argv) {
	int i, fd;

	if (argc <= 1) {
		wc(0, "");
		_exit(0);
	}
	for (i = 1; i < argc; ++i) {
		if ((fd = open(argv[i], 0, 0)) < 0) {
			_exit(3);
		}
		wc(fd, argv[i]);
		close(fd);
	}
	return 0;
}
