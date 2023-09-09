#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
	i32 pid = fork();
	if (pid == 0) {
		printf("I'm a child\n");
		exec("/bin/test");
		printf("After exec\n");
	}
	i32 y = wait(NULL);
	printf("Parent malloc\n");
	u8 *x = (u8 *)malloc(4);
	x[0] = 1;
	x[1] = 2;
	x[2] = 3;
	x[3] = 4;
	for (u32 i = 0; i < 4; ++i) {
		printf("%d\n", x[i]);
	}
	free(x);
	printf("PID of terminated child - %d\n", y);
	for (;;);
	return 0;
}
