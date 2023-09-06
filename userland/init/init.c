#include <stdio.h>
#include <unistd.h>

int main() {
	i32 x = fork();
	printf("%d\r\n");
	for (;;);
	return 0;
}
