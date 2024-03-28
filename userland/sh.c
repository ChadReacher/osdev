#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

static i8 cwd[256];

struct cmd {
	i32 type;
	void *data;
};

struct exec_cmd {
	i32 argc;
	i8 *argv[10];
};

static i8 buf[1024];

void print_prompt();
i8 *read_input();
struct cmd *parse_cmd(i8 *input);
void run_cmd(struct cmd *);
void builtin_cd(i8 *path);

void sigpass(i32 s) {

}

i32 main() {
	i8 *input;
	struct cmd *cmd;
	sigaction_t act = {
		.sa_handler = sigpass,
		.sa_mask = 0,
		.sa_flags = 0
	};
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);
	printf("\033[2J\033[;H");
	for (;;) {
		print_prompt();
		memset(buf, 0, 1024);
		input = read_input();
		if (strlen(input) == 0) {
			continue;
		}
		if (strlen(input) > 2 && strncmp(input, "cd", 2) == 0) { 
			i8 *path = input + 3;
			builtin_cd(path);
			continue;
		}
		cmd = parse_cmd(input);
		if (cmd == NULL) {
			printf("Invalid input\n");
			continue;
		}
		i32 chd, s, pid;
		if ((chd = fork()) == 0) {
			run_cmd(cmd);
		}
		do {
			pid = waitpid(chd, &s, 0);
		} while (pid != chd);
		if (WIFSIGNALED(s)) {
			printf("signaled\n");
			switch (WTERMSIG(s)) {
				case SIGINT:
					break;
				default:
					printf("Terminated due to signal %d\n", WTERMSIG(s));
					break;
			}
		}

		free(cmd->data);
		free(cmd);
	}

	return 0;
}

void print_prompt() {
	i8 *buf = getcwd(cwd, 256);
	if (buf) {
		printf("[%s]$ ", cwd);
	} else {
		printf("got error\n");
	}
}

i8 *read_input() {
	u8 c;
	u32 len = 0;
	bool newline = false;
	while (!newline) {
		c = getchar();
		if (c == '\n') {
			buf[len] = '\0';
			return buf;
		} else if (c == 127) {
			if (len > 0) {
				buf[--len] = 'x';
			}
		} else if (c == '\t') {
			continue;
		} else {
			if (c < ' ') {
				continue;
			}
			buf[len++] = c;
		}
	}
	return buf;
}

static i8 whitespaces[] = " \t\r\n";

struct cmd *parse_cmd(i8 *input) {
	struct exec_cmd *execmd;
	struct cmd *cmd;
	i8 **argv;
	i32 i_argc, i;
	i8 *s, *ends, *arg;
	if (!input || !strlen(input)) {
		return NULL;
	}
	argv = malloc(10 * sizeof(i8 *));
	i_argc = 0;

	s = input;
	ends = input + strlen(input);

	/* Skip whitespaces in the beginning */
	while (s < ends && strchr(whitespaces, *s) != NULL) {
		++s;
	}

	while ((arg = strsep(&s, " ")) != NULL) {
		if (strcmp(arg, "") == 0) {
			continue;
		}
		argv[i_argc] = strdup(arg);
		++i_argc;
		if (i_argc == 10) {
			printf("error: max args - 10\n");
			for (i = 0; i < i_argc; ++i) {
				free(argv[i]);
			}
			free(argv);
			return NULL;
		}
	}
	execmd = malloc(sizeof(struct exec_cmd));
	memset(execmd, 0, sizeof(struct exec_cmd));
	execmd->argc = i_argc;
	argv[i_argc] = NULL;
	memcpy(execmd->argv, argv, 10 * sizeof(i8 *));
	for (i = 0; i < i_argc; ++i) {
		free(argv[i]);
	}
	free(argv);

	cmd = malloc(sizeof(struct cmd));
	cmd->type = 1;
	cmd->data = (void *)execmd;
	return cmd;
}

void run_cmd(struct cmd *cmd) {
	i32 err;
	if (!cmd) {
		return;
	}
	if (cmd->type == 1) {
		struct exec_cmd *exec = (struct exec_cmd *)cmd->data;
		err = execvp(exec->argv[0], exec->argv);
		if (err) {
			free(exec);
			free(cmd);
			perror("sh failed");
		}
		_exit(-1);
	}
}

void builtin_cd(i8 *path) {
	if (!path || strlen(path) == 0) {
		return;
	}
	if (chdir(path) == -1) {
		printf("cd: could not change directory\n");
		return;
	}
}
