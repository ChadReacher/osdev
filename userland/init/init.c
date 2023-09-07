#include <stdio.h>
#include <unistd.h>

int main() {
	i32 x = fork();
	printf("%d\r\n");
	if (x == 0) {
		printf("I'm a child\n");
	} else {
		printf("I'm a parent\n");
		exec("/bin/test");
		printf("After exec?\n");
	}
	for (;;);
	return 0;
}
