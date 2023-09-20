#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>

int main(i32 argc, i8 *argv[]) {
	printf("argc - %x\n", argc);
	for (i32 i = 0; i < argc; ++i) {
		printf("argv[%d] - %s\n", i, argv[i]);
	}
	printf("Enter character: ");
	u8 m = getchar();
	putchar(m);
	putchar('\n');
	printf("Test malloc:\n");
	u8 *x = (u8 *)malloc(4);
	x[0] = 1;
	x[1] = 2;
	x[2] = 3;
	x[3] = 4;
	for (u32 i = 0; i < 4; ++i) {
		printf("%d\n", x[i]);
	}
	printf("Before free\n");
	free(x);
	for (u32 i = 0; i < 10; ++i) {
		printf("friends\n");
	}
	return 123;
}
