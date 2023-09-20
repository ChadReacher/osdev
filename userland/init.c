#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

extern i8 **environ;

int main(i32 argc, i8 *argv[], i8 *envp[]) {
	printf("argc - %d\n", argc);
	for (i32 i = 0; i < argc; ++i) {
		printf("argv[%d] - %s\n", argv[i]);
	}
    printf("envp[0] - '%s'\n", envp[0]);
	i32 pid = fork();
	if (pid == 0) {
		printf("I'm a child\n");
        printf("envp[0] - '%s'\n", environ[0]);
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
