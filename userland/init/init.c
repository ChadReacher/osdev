#include <stdio.h>
#include <unistd.h>

int main() {
	i32 x = fork();
	if (x == 0) {
		printf("I'm a child\n");
		exec("/bin/test");
		printf("After exec\n");
	}
	i32 y = waitpid(-1, NULL, 0);
	printf("PID of terminated child - %d\n", y);
	for (;;);
	return 0;
}
