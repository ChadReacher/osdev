#include <unistd.h>
#include <stdio.h>

int main() {
	test("Hello, world from another program in ELF.\n");
	printf("Hello, world from user program using libc 'printf'\n");
	return 0;
}
