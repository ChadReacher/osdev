#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(i32 argc, i8 **argv, i8 **envp) {
	printf("argc - %x\n", argc);
	for (i32 i = 0; i < argc; ++i) {
		printf("argv[%d] - %s\n", i, argv[i]);
	}
	printf("TEST message with infinite loop, pid - %d\n", getpid());
	printf("Test malloc:\n");
	putchar('h');
	putchar('i');
	putchar('\n');
	u8 m = getchar();
	putchar(m);
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
	printf("Exit on 5 out of 10\n");
	for (u32 i = 0; i < 10; ++i) {
		//if (i == 5) {
		//	exit(123);
		//}
		printf("friends\n");
	}
	//for (;;);
	return 123;
}
