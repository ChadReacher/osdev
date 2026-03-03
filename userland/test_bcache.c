#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

static void child(void);
static void parent(void);

int main(void) {
    printf("Testing block cache\n");
    for (int i = 0; i < 10; ++i) {
        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            _exit(1);
        } else if (pid == 0) {
            child();
        }
    }
    parent();

    return 0;
}

void child(void) {
    int err;
    char buffer[512] = {0};

    int fd = open("/home/file", O_RDONLY, 0);
    for (int i = 0; i < 100; ++i) {
        err = lseek(fd, 0, SEEK_SET);
        if (err < 0) {
            perror("lseek failed");
            _exit(1);
        }

        err = read(fd, buffer, 512);
        if (err != 512) {
            printf("read() failed, didn't read all buffer (only %d) - %s\n", err, strerror(errno));
            _exit(1);
        }
    }

    close(fd);

    _exit(1);
}

void parent(void) {
    int err;
    char buffer[512] = {0};

    int fd = open("/home/file", O_RDONLY, 0);
    for (int i = 100; i > 0; --i) {
        err = lseek(fd, 0, SEEK_SET);
        if (err < 0) {
            perror("lseek failed");
            _exit(1);
        }

        err = read(fd, buffer, 512);
        if (err != 512) {
            printf("read() failed, didn't read all buffer (only %d) - %s\n", err, strerror(errno));
            _exit(1);
        }
    }

    close(fd);

    int pid;
    do {
        pid = wait(NULL);
    } while (pid != -1);
}