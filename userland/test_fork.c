#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

// Aligned with internal kernel process array
#define PROCS 32

static void test_proc_exhaustion(void);
static void test_many_procs(void);

int main(void) {
    printf("Test process table exhaustion\n");
    test_proc_exhaustion();
    printf("Test spawning many processes\n");
    test_many_procs();
    return 0;
}

void test_proc_exhaustion(void) {
    // System already has 4 procs: 0 (kidle), 1 (init), 2 (sh), 3 (us).
    for (int i = 0; i < PROCS - 4; ++i) {
        int pid = fork();
        if (pid < 0) {
            perror("test fork failed: fork failed");
            return;
        } else if (pid == 0) {
            sleep(1);
            _exit(0);
        }
    }

    int pid = fork();
    if (pid < 0) {
        if (errno == EAGAIN) {
            printf("Succesfully reached process exhaustion\n");
        } else {
            printf("Fork failed but for a different reason: %d\n", errno);
        }
    } else if (pid == 0) {
        printf("Error: fork didn't fail\n");
        _exit(1);
    }

    pid = 0;
    do {
        pid = wait(NULL);
    } while (pid != -1);
}

void test_many_procs(void) {
    int nprocs = 50;
    for (int i = 0; i < nprocs; ++i) {
        int pid = fork();
        if (pid < 0) {
            if (errno == EAGAIN) continue;
            printf("Unexpected error from failed fork: %d", errno);
            _exit(1);
        } else if (pid == 0) {
            sleep(1);
            _exit(0);
        } else {
            continue;
        }
    }

    int pid = 0;
    do {
        pid = wait(NULL);
    } while (pid != -1);
}
