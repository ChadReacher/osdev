#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

extern i8 **environ;

int main() {
	i32 pid = fork();
	if (pid == 0) {
		i8 *m[] = {"/bin/sh", 0};
		execv("/bin/sh", m);
		printf("After exec\n");
	}
	i32 wstatus = 0;
	i32 y = wait(&wstatus);
	printf("PID of terminated child - %d\n", y);
	printf("Return value of terminated child - %d\n", wstatus);
	return 0;
}
