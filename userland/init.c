#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

int main(void) {
    i32 pid, fd;
    i32 status = 0;
    i8 *vector[] = { "/bin/sh", NULL };

    fd = open("/dev/tty0", O_RDWR, 0);
    dup(fd);
    dup(fd);

    pid = fork();
    if (pid == 0) {
        execv("/bin/sh", vector);
        printf("unreachable\n");
        _exit(1);
    }

    while (pid != -1) {
        pid = wait(&status);
        if (pid > 0) {
            printf("Child %d terminated, status code = %d\n", pid, status);
        }
    }

    return 0;
}
