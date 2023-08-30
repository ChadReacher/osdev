#include <stdio.h>
#include <unistd.h>

int main() {
	printf("Hello, world from userland!\n");
	for (u32 i = 0; i < 5; ++i) {
		printf("Hello - %d\r\n", i);
	}
	exec("/bin/test");
	printf("after exec\n");
	for (;;);
	return 0;
}
