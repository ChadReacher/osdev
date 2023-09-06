#include <stdio.h>
#include <unistd.h>

int main() {
	for (;;) {
		printf("bye\n");
	}
	for (u32 i = 0; i < 300; ++i) {
		printf("%d - push ups\n", i);
	}
	//printf("TEST message with infinite loop\n");
	//for (u32 i = 0; i < 5; ++i) {
	//	printf("friends\n");
	//}
	for (;;);
	return 0;
}
