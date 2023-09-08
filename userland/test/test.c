#include <stdio.h>
#include <unistd.h>

int main() {
	printf("TEST message with infinite loop\n");
	for (u32 i = 0; i < 100; ++i) {
		if (i == 75) {
			exit(123);
		}
		printf("friends\n");
	}
	for (;;);
	return 0;
}
