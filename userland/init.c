#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

int main(void) {
	i8 *m[] = {"/bin/sh", 0};
	i32 pid, stat_loc;
	i32 fd = open("/dev/tty0", O_RDWR, 0);
	dup(fd);
	dup(fd);

	pid = fork();
	stat_loc = 0;
	if (pid == 0) {
		close(0);
		close(1);
		close(2);
		setsid();
		open("/dev/tty0", O_RDWR, 0);
		dup(0);
		dup(0);
		execv("/bin/sh", m);
		printf("After exec\n");
	}
	do {
		pid = wait(&stat_loc);
		printf("PID of terminated child - %d\n", pid);
		printf("Return value of terminated child - %d\n", stat_loc);
	} while (pid != -1);
	return 0;
}
