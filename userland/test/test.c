#include <stdio.h>

int main() {
	printf("TEST message with infinite loop\n");
	for (u32 i = 0; i < 120; ++i) {
		printf("friends\n");
	}
	for (;;);
	return 0;
}
