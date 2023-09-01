#include <stdio.h>
#include <unistd.h>

int main() {
	i32 x = fork();
	printf("Return - %d\n", x);
	if (x == 0) {
		printf("We are child\r\n");
		exec("/bin/test");
		printf("after exec\n");
	} else {
		printf("We are parent\r\n");
		yield();
	}
	for (;;);
	return 0;
}
