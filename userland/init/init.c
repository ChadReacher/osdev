#include <stdio.h>
#include <unistd.h>

int main() {
	i32 x = fork();
	if (x == 0) {
		i32 y = fork();
		if (y == 0) {
			for (u32 i = 0; i < 100000; ++i) {
				printf("child2 - %d\n", i);
			}
		} else {
			for (u32 i = 0; i < 100000; ++i) {
				if (i == 50) {
					exit();
				}
				printf("child1 - %d\n", i);
			}
		}
	} else {
		for (u32 i = 0; i < 100000; ++i) {
			printf("parent - %d\n", i);
		}
	}
	for (;;);
	return 0;
}
