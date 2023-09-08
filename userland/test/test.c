#include <stdio.h>
#include <unistd.h>

int main() {
	printf("TEST message with infinite loop, pid - %d\n", getpid());
	for (u32 i = 0; i < 20; ++i) {
		if (i == 10) {
			exit(123);
		}
		printf("friends\n");
	}
	for (;;);
	return 0;
}
