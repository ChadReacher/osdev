#include <stdio.h>
#include <unistd.h>

int main() {
	//i32 x = fork();
	//if (x == 0) {
	//	printf("I'm a child\n");
	//	exec("/bin/test");
	//	printf("After exec\n");
	//}
	//i32 y = wait();
	//printf("PID of terminated child - %d\n", y);
	i32 x = fork();
	if (x == 0) {
		i32 y = fork();
		if (y == 0) {
			for (u32 i = 0; i < 100000; ++i) {
				if (i == 100) {
					exit();
				}
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
