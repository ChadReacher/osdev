#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

void sigquit_handler(int sig) {
    printf("SIGQUIT handler - %d\n", sig);
}

void sigint_handler(int sig) {
    printf("SIGINT handler - %d\n", sig);
}

void sigill_handler(int sig) {
    printf("SIGILL handler - %d\n", sig);
    _exit(0);
}

void sigtrap_handler(int sig) {
    printf("SIGTRAP handler - %d\n", sig);
}

void sighup_handler(int sig) {
    printf("SIGHUP handler - %d\n", sig);
    _exit(0);
}

void sigabrt_handler(int sig) {
    printf("SIGABRT handler - %d\n", sig);
}

void sigfpe_handler(int sig) {
    printf("SIGFPE handler - %d\n", sig);
    _exit(0);
}

void sigusr1_handler(int sig) {
    printf("SIGUSR1 handler - %d\n", sig);
}

void sigusr2_handler(int sig) {
    printf("SIGUSR2 handler - %d\n", sig);
}

void sigsegv_handler(int sig) {
    printf("SIGSEGV handler - %d\n", sig);
    _exit(0);
}

void sigpipe_handler(int sig) {
    printf("SIGPIPE handler - %d\n", sig);
}

void sigalrm_handler(int sig) {
    printf("SIGALRM handler - %d\n", sig);
}

void sigterm_handler(int sig) {
    printf("SIGTERM handler - %d\n", sig);
}

void sigchld_handler(int sig) {
    printf("SIGCHLD handler - %d\n", sig);
}

void sigcont_handler(int sig) {
    printf("SIGCONT handler - %d\n", sig);
}

void sigtstp_handler(int sig) {
    printf("SIGTSTP handler - %d\n", sig);
}

int main(void) {
    printf("Testing signals\n");

    {
        printf("Testing SIGHUP signal\n");
        int sync_fd[2];
        int err = pipe(sync_fd);
        if (err < 0) {
            perror("pipe failed");
            _exit(1);
        }

        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            _exit(1);
        } else if (pid == 0) {
            int err = setsid();
            if (err < 0) {
                perror("setsid failed");
                _exit(1);
            }

            int pid = fork();
            if (pid < 0) {
                perror("fork failed");
                _exit(1);
            } else if (pid == 0) {
                sigaction_t sighup_act = { sighup_handler, 0, 0 };
                sigemptyset(&sighup_act.sa_mask);
                sigaction(SIGHUP, &sighup_act, NULL);

                close(sync_fd[0]);
                write(sync_fd[1], "!", 1);

                pause();
            }
            char buf = 0;
            close(sync_fd[1]);
            read(sync_fd[0], &buf, 1);
            _exit(0);
        }

        do {
            pid = wait(NULL);
        } while (pid != -1);
    }

    {
        printf("Testing SIGINT signal (need to press Ctrl-C)\n");
        sigaction_t sigint_act = { sigint_handler, 0, 0 };
        sigemptyset(&sigint_act.sa_mask);
        sigaction(SIGINT, &sigint_act, NULL);
        pause();
    }

    {
        printf("Testing SIGQUIT signal (need to press Ctrl-\\)\n");
        sigaction_t sigquit_act = { sigquit_handler, 0, 0 };
        sigemptyset(&sigquit_act.sa_mask);
        sigaction(SIGQUIT, &sigquit_act, NULL);
        pause();
    }

    {
        printf("Testing SIGILL signal\n");
        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            _exit(1);
        } else if (pid == 0) {
            sigaction_t sigill_act = { sigill_handler, 0, 0 };
            sigemptyset(&sigill_act.sa_mask);
            sigaction(SIGILL, &sigill_act, NULL);

            // Generates an invalid opcode.
            __asm__ volatile ("ud2" ::: "memory");
        }
        int err = waitpid(pid, NULL, 0);
        if (err < 0) {
            perror("waitpid failed");
            _exit(1);
        }
    }

    {
        printf("Testing SIGTRAP signal\n");
        sigaction_t sigtrap_act = { sigtrap_handler, 0, 0 };
        sigemptyset(&sigtrap_act.sa_mask);
        sigaction(SIGTRAP, &sigtrap_act, NULL);

        __asm__ volatile ("int3" ::: "memory");
    }

    {
        printf("Testing SIGABRT signal\n");
        sigaction_t sigabrt_act = { sigabrt_handler, 0, 0 };
        sigemptyset(&sigabrt_act.sa_mask);
        sigaction(SIGABRT, &sigabrt_act, NULL);

        kill(getpid(), SIGABRT);
    }

    {
        printf("Testing SIGFPE signal\n");
        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            _exit(1);
        } else if (pid == 0) {
            sigaction_t sigfpe_act = { sigfpe_handler, 0, 0 };
            sigemptyset(&sigfpe_act.sa_mask);
            sigaction(SIGFPE, &sigfpe_act, NULL);

            int x = 1;
            int y = 0;
            __asm__ volatile ("divl %2" : "=a"(x) : "a"(x), "r"(y));

            pause();
        }
        int err = waitpid(pid, NULL, 0);
        if (err < 0) {
            perror("waitpid failed");
            _exit(1);
        }
    }

    {
        printf("Testing SIGKILL signal\n");
        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            _exit(1);
        } else if (pid == 0) {
            pause();
        }

        sleep(1);
        kill(pid, SIGKILL);

        int status;
        int err = waitpid(pid, &status, 0);
        if (err < 0) {
            perror("waitpid failed");
            _exit(1);
        }
        if (WIFSIGNALED(status)) {
            if (WTERMSIG(status) == SIGKILL) {
                printf("Child process successfully exited due to SIGKILL\n");
            } else {
                printf("error: expected child process to exit due to SIGKILL but exited to %d\n", WTERMSIG(status));
            }
        } else {
            printf("error: child process exited not by signal\n");
            _exit(1);
        }
    }

    {
        printf("Testing SIGUSR1 and SIGUSR2 signal\n");
        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            _exit(1);
        } else if (pid == 0) {
            sigaction_t sigusr1_act = { sigusr1_handler, 0, 0 };
            sigemptyset(&sigusr1_act.sa_mask);
            sigaction(SIGUSR1, &sigusr1_act, NULL);

            sigaction_t sigusr2_act = { sigusr2_handler, 0, 0 };
            sigemptyset(&sigusr2_act.sa_mask);
            sigaction(SIGUSR2, &sigusr2_act, NULL);

            pause();

            pause();

            _exit(1);
        }

        sleep(1);
        kill(pid, SIGUSR1);

        sleep(1);
        kill(pid, SIGUSR2);

        int err = waitpid(pid, NULL, 0);
        if (err < 0) {
            perror("waitpid failed");
            _exit(1);
        }
    }

    {
        printf("Testing SIGSEGV signal\n");
        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            _exit(1);
        } else if (pid == 0) {
            sigaction_t sigsegv_act = { sigsegv_handler, 0, 0 };
            sigemptyset(&sigsegv_act.sa_mask);
            sigaction(SIGSEGV, &sigsegv_act, NULL);

            *(volatile int *)((0xFFFF1111)) = 123;
        }

        int err = waitpid(pid, NULL, 0);
        if (err < 0) {
            perror("waitpid failed");
            _exit(1);
        }
    }

    {
        printf("Testing SIGPIPE signal\n");
        int sync_fd[2];
        int err = pipe(sync_fd);
        if (err < 0) {
            perror("pipe failed");
            _exit(1);
        }
        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            _exit(1);
        } else if (pid == 0) {
            sigaction_t sigpipe_act = { sigpipe_handler, 0, 0 };
            sigemptyset(&sigpipe_act.sa_mask);
            sigaction(SIGPIPE, &sigpipe_act, NULL);

            sleep(1);
            close(sync_fd[0]);
            write(sync_fd[1], "!", 1);

            _exit(0);
        }

        close(sync_fd[1]);
        close(sync_fd[0]);

        err = waitpid(pid, NULL, 0);
        if (err < 0) {
            perror("waitpid failed");
            _exit(1);
        }
    }

    {
        printf("Testing SIGALRM signal (set timer on 2 seconds)\n");
        sigaction_t sigalrm_act = { sigalrm_handler, 0, 0 };
        sigemptyset(&sigalrm_act.sa_mask);
        sigaction(SIGALRM, &sigalrm_act, NULL);

        alarm(2);
        pause();
    }

    {
        printf("Testing SIGTERM signal\n");
        sigaction_t sigterm_act = { sigterm_handler, 0, 0 };
        sigemptyset(&sigterm_act.sa_mask);
        sigaction(SIGTERM, &sigterm_act, NULL);

        kill(getpid(), SIGTERM);
    }

    {
        printf("Testing SIGCHLD signal\n");
        sigaction_t sigchld_act = { sigchld_handler, 0, 0 };
        sigemptyset(&sigchld_act.sa_mask);
        sigaction(SIGCHLD, &sigchld_act, NULL);

        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            _exit(1);
        } else if (pid == 0) {
            sleep(1);
            _exit(0);
        }

        pause();
        wait(NULL);
        sigchld_act.sa_handler = SIG_DFL;
        sigaction(SIGCHLD, &sigchld_act, NULL);
    }

    {
        printf("Testing SIGSTOP and SIGCONT signal\n");

        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            _exit(1);
        } else if (pid == 0) {
            // sigaction_t sigcont_act = { sigcont_handler, 0, 0 };
            // sigemptyset(&sigcont_act.sa_mask);
            // sigaction(SIGCONT, &sigcont_act, NULL);

            pause();
            printf("Child process successfully continued due to SIGCONT\n");

            _exit(0);
        }

        sleep(1);
        kill(pid, SIGSTOP);

        int status;
        int err = waitpid(pid, &status, WUNTRACED);
        if (err < 0) {
            perror("waitpid failed");
            _exit(1);
        }
        if (WIFSTOPPED(status)) {
            if (WSTOPSIG(status) == SIGSTOP) {
                printf("Child process successfully stopped due to SIGSTOP\n");
            } else {
                printf("error: expected child process to stop due to SIGSTOP but stopped to %d\n", WSTOPSIG(status));
            }
        } else {
            printf("error: child process stopped not by signal\n");
            _exit(1);
        }

        kill(pid, SIGCONT);

        err = waitpid(pid, NULL, 0);
        if (err < 0) {
            perror("waitpid failed");
            _exit(1);
        }
    }

    {
        printf("Testing SIGTSTP signal (need to press Ctrl-Z)\n");
        sigaction_t sigtstp_act = { sigtstp_handler, 0, 0 };
        sigemptyset(&sigtstp_act.sa_mask);
        sigaction(SIGTSTP, &sigtstp_act, NULL);
        pause();
    }

    printf("Skipping SIGTTIN signal test (unimplemented)\n");
    printf("Skipping SIGTTOU signal test (unimplemented)\n");

    {
        printf("Testing sending the ignored signal\n");
        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            _exit(0);
        } else if (pid == 0) {
            sigaction_t sigusr1_act = { SIG_IGN, 0, 0 };
            sigemptyset(&sigusr1_act.sa_mask);
            sigaction(SIGUSR1, &sigusr1_act, NULL);
            pause();
            printf("unreachable\n");
            _exit(0);
        }
        sleep(1);
        kill(pid, SIGUSR1);

        sleep(1);
        int status;
        int err = waitpid(pid, &status, WNOHANG);
        if (err == 0) {
            printf("Child didn't receive SIGUSR1 and is asleep as expected\n");
            kill(pid, SIGKILL);
            int err = waitpid(pid, NULL, 0);
            if (err < 0) {
                perror("waitpid failed");
                _exit(1);
            }
        } else if (err == pid) {
            printf("error: child process exited\n");
            _exit(1);
        } else {
            perror("waitpid failed");
            _exit(1);
        }
    }

    return 0;
}