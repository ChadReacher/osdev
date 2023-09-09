#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
	//printf("Start program\n");
	//u8 *p = (u8 *)malloc(5);
	//printf("Allocated 5 bytes at %p\n", p);
	//for (u8 i = 0; i < 5; ++i) {
	//	p[i] = 4;
	//}
	//for (u8 i = 0; i < 5; ++i) {
	//	printf("%d\n", p[i]);
	//}
	//printf("\n");
	//p = (u8 *)realloc(p, 150);
	//printf("After REALLOCATING TO 15 bytes at %p\n", p);
	//for (u8 i = 0; i < 15; ++i) {
	//	printf("%d\n", p[i]);
	//}
	//printf("\n");
	//for (u8 i = 5; i < 16; ++i) {
	//	p[i] = i;
	//}
	//for (u8 i = 0; i < 15; ++i) {
	//	printf("%d\n", p[i]);
	//}
	//printf("\n");

	//u8 *p2 = (u8 *)malloc(15);
	//printf("Allocated 15 bytes at %p\n", p2);
	//for (u8 i = 1; i < 6; ++i) {
	//	p2[i - 1] = i;
	//}
	//for (u8 i = 0; i < 5; ++i) {
	//	printf("%d\n", p2[i]);
	//}
	//printf("\n");

	//free((void*)p);
	//free((void*)p2);
	//printf("Freed 150 bytes with p\n");

	i32 pid = fork();
	if (pid == 0) {
		printf("I'm a child\n");
		exec("/bin/test");
		printf("After exec\n");
	}
	i32 y = wait(NULL);
	printf("Parent malloc\n");
	u8 *x = (u8 *)malloc(4);
	x[0] = 1;
	x[1] = 2;
	x[2] = 3;
	x[3] = 4;
	for (u32 i = 0; i < 4; ++i) {
		printf("%d\n", x[i]);
	}
	free(x);
	printf("PID of terminated child - %d\n", y);
	for (;;);
	return 0;
}
