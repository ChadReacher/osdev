#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

static void test_process_fd_table(void);
static void test_system_fd_table(void);

int main(void) {
    printf("Testing process-wide open fd table\n");
    test_process_fd_table();
    printf("Testing system-wide open fd table\n");
    test_system_fd_table();
    return 0;
}

// Aligned with internal kernel fd table per process
#define NR_OPEN 20

void test_process_fd_table(void) {
    i32 err;

    // This process already has 3 opened fd: stdin, stdout and stderr.
    for (int i = 0; i < NR_OPEN - 3; ++i) {
        i32 fd = open("/dev/null", O_RDONLY, 0);
        if (fd < 0) {
            perror("open failed");
            _exit(1);
        }
    }

    err = open("/dev/null", O_RDONLY, 0);
    if (err == -1) {
        if (errno == EMFILE) {
            printf("We successfully reached the process-wide open fd limit\n");
        } else {
            printf("error: expected open(2) to fail for EMFILE but it failed for %s\n", strerror(errno));
            _exit(1);
        }
    } else {
        printf("error: expected open(2) to fail but it succeeded\n");
        _exit(1);
    }

    // Close any open fd to free the space
    err = close(3);
    if (err != 0) {
        perror("close failed");
        _exit(1);
    }

    // We should have the space to open
    int fd = open("/dev/null", O_RDONLY, 0);
    if (fd < 0) {
        printf("error: expected open(2) to succeed but it failed: %s\n", strerror(errno));
        _exit(1);
    }

    for (int i = 3; i < NR_OPEN; ++i) {
        err = close(i);
        if (err != 0) {
            perror("close failed");
            _exit(1);
        }
    }
}

// Aligned with internal kernel fd table on system
#define NR_FILE 32

void test_system_fd_table(void) {
    // we already opened /dev/tty0 + 7 * 4 + 2
    for (int i = 0; i < 7; ++i) {
        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            _exit(1);
        } else if (pid == 0) {
            for (int j = 0; j < 4; ++j) {
                i32 fd = open("/dev/null", O_RDONLY, 0);
                if (fd < 0) {
                    perror("open failed");
                    _exit(1);
                }
            }
            sleep(3);
            _exit(0);
        } else {
            continue;
        }
    }
    sleep(2);

    for (int i = 0; i < 3; ++i) {
        i32 fd = open("/dev/null", O_RDONLY, 0);
        if (fd < 0) {
            perror("open failed");
            _exit(1);
        }
    }

    i32 err = open("/dev/null", O_RDONLY, 0);
    if (err == -1) {
        if (errno == ENFILE) {
            printf("We successfully reached the system-wide open fd limit\n");
        } else {
            printf("error: expected open(2) to fail for ENFILE but it failed for %s\n", strerror(errno));
            _exit(1);
        }
    } else {
        printf("error: expected open(2) to fail but it succeeded\n");
        _exit(1);
    }

    int pid;
    do {
        pid = wait(NULL);
    } while (pid != -1);
}