#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>

int main(i32 argc, i8 *argv[]) {
	i32 i;
	printf("argc - %x\n", argc);
	for (i = 0; i < argc; ++i) {
		printf("argv[%d] - %s\n", i, argv[i]);
	}
	for (i = 0; i < 30; ++i) {
		printf("%d\n", i);
		sleep(1);
	}
	return 123;
}
