#include <stdio.h>
#include <unistd.h>

int main() {
	i32 x = fork();
	printf("%d\n", x);
	if (x == 0) {
		for (u32 i = 0; i < 300; ++i) {
			//if (i == 50) {
			//	exit();
			//}
			printf("%d - pull ups\n", i);
		}
	} else {
		for (u32 i = 0; i < 300; ++i) {
			printf("%d - push ups\n", i);
		}
	}
	//printf("Return - %d\n", x);
	//if (x == 0) {
	//	printf("We are child\r\n");
	//	exec("/bin/test");
	//	printf("after exec\n");
	//} else {
	//	printf("We are parent\r\n");
	//}
	for (;;);
	return 0;
}
