#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
	printf("TEST message with infinite loop, pid - %d\n", getpid());
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
	printf("Exit on 5 out of 10\n");
	for (u32 i = 0; i < 10; ++i) {
		if (i == 5) {
			exit(123);
		}
		printf("friends\n");
	}
	for (;;);
	return 0;
}
