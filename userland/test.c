#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(i32 argc, i8 *argv[], i8 *envp[]) {
	for(u32 i = 0; i < 200; ++i) {
		if (i == 50) {
			sleep(5);
		}
		printf("child - %d\n", i);
	}
	//printf("argc - %x\n", argc);
	//for (i32 i = 0; i < argc; ++i) {
	//	printf("argv[%d] - %s\n", i, argv[i]);
	//}
	//printf("TEST message with infinite loop, pid - %d\n", getpid());
	//printf("Test malloc:\n");
	//putchar('h');
	//putchar('i');
	//putchar('\n');
	////u8 m = getchar();
	////putchar(m);
	//u8 *x = (u8 *)malloc(4);
	//x[0] = 1;
	//x[1] = 2;
	//x[2] = 3;
	//x[3] = 4;
	//for (u32 i = 0; i < 4; ++i) {
	//	printf("%d\n", x[i]);
	//}
	//printf("Before free\n");
	//free(x);
	//for (u32 i = 0; i < 10; ++i) {
	//	printf("friends\n");
	//}
	return 123;
}
