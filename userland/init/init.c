#include <stdio.h>
#include <unistd.h>

int main() {
	printf("My PID - %d\n", getpid());
	i32 x = fork();
	if (x == 0) {
		printf("I'm a child\n");
		exec("/bin/test");
		printf("After exec\n");
	}
	i32 y = wait(NULL);
	printf("PID of terminated child - %d\n", y);
	for (;;);
	return 0;
}
