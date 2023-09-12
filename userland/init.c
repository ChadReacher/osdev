#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(i32 argc, i8 *argv[], i8 *envp[]) {
	printf("argc - %d\n", argc);
	for (i32 i = 0; i < argc; ++i) {
		printf("argv[%d] - %s\n", argv[i]);
	}
	i32 pid = fork();
	if (pid == 0) {
		printf("I'm a child\n");
		i8 *m[] = {"/bin/sh", 0};
		execve("/bin/sh", m, 0);
		printf("After exec\n");
	}
	i32 wstatus = 0;
	i32 y = wait(&wstatus);
	printf("PID of terminated child - %d\n", y);
	printf("Return value of terminated child - %d\n", wstatus);
	for (;;);
	return 0;
}
