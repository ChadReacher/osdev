#include <stdio.h>
#include <unistd.h>

int main() {
	//for (u32 i = 0; i < 100000; ++i) {
	//	printf("Parent\n");
	//}
	printf("TEST message with infinite loop\n");
	for (u32 i = 0; i < 5; ++i) {
		printf("friends\n");
	}
	exit();
	for (;;);
	return 0;
}
