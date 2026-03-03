#include "sys/stat.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

static void test_inode_cache_exhaustion(void);
static void test_inode_cache_contention(void);

int main(void) {
    // Inode slot exhaustion
    printf("Testing inode cache exhaustion\n");
    test_inode_cache_exhaustion();
    printf("Testing inode cache contention\n");
    test_inode_cache_contention();
    return 0;
}

static void child_contention(void) {
    struct stat sb;
    unsigned int expected_size = 2273;

    for (int i = 0; i < 200; ++i) {
        int err = stat("/home/file", &sb);
        if (err < 0) {
            perror("stat failed");
            _exit(1);
        }
        if (sb.st_size != expected_size) {
            printf("error: read wrong size; expected size = %d, actual = %d\n",
            expected_size, sb.st_size);
        }
    }
}

// Aligned with kernel internal inode cache
// NOTE: Decrease the NR_INODES in kernel to ease the testing
#define NR_INODES 6

struct file_size {
    const char *file;
    unsigned int size;
};

void test_inode_cache_exhaustion(void) {
    struct file_size files[NR_INODES] = {
        { "/bin/test", 34148 },
        { "/bin/ls", 39072 },
        { "/bin/link", 33968 },
        { "/bin/readlink", 34180 },
        { "/bin/cat", 35712 },
        { "/bin/rm", 34024 }
    };
    // 1st inode is root dir
    // 2nd inode is tty0
    // 3rd inode is a temporary parent dir (/bin)
    for (int i = 0; i < NR_INODES - 3; ++i) {
        int err = open(files[i].file, O_RDONLY, 0);
        if (err < 0) {
            perror("open failed");
            _exit(1);
        }
    }

    int err = open("/home/file", O_RDONLY, 0);
    if (err < 0) {
        if (errno == ENFILE) {
            printf("Succesfully reached inode table exhaustion\n");
        } else {
            printf("error: expected open(2) to fail with ENOMEM but it failed with %s\n", strerror(errno));
            _exit(1);
        }
    } else {
        printf("error: expected open(2) to fail but it succeeded\n");
        _exit(1);
    }
    for (int i = 0; i < NR_INODES - 3; ++i) {
        i32 err = close(3 + i);
        if (err < 0) {
            perror("close failed");
            _exit(1);
        }
    }
}

void test_inode_cache_exhaustion2(void) {
    struct file_size files[NR_INODES] = {
        { "/bin/test", 34148 },
        { "/bin/ls", 39072 },
        { "/bin/link", 33968 },
        { "/bin/readlink", 34180 },
        { "/bin/cat", 35712 },
        { "/bin/rm", 34024 }
    };
    for (int i = 0; i < NR_INODES; ++i) {
        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            _exit(1);
        } else if (pid == 0) {
            struct stat sb;
            int err;

            err = stat(files[i].file, &sb);
            if (err < 0) {
                perror("stat failed");
                _exit(1);
            }
            if (sb.st_size != files[i].size) {
                printf("error: read wrong size; expected size = %d, actual = %d\n", files[i].size, sb.st_size);
                _exit(1);
            }

            sleep(3);

            _exit(1);
        }
    }

    int pid;
    do {
        pid = wait(NULL);
    } while (pid != -1);
}

void test_inode_cache_contention(void) {
    for (int i = 0; i < 10; ++i) {
        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            _exit(1);
        } else if (pid == 0) {
            child_contention();
            _exit(1);
        }
    }

    int pid;
    do {
        pid = wait(NULL);
    } while (pid != -1);
}