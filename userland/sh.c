#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

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

i32 main() {
	for (;;) {
		print_prompt();
		memset(buf, 0, 1024);
		i8 *input = read_input();
		struct cmd *cmd = parse_cmd(input);
		if (cmd == NULL) {
			printf("Invalid input\n");
			continue;
		}
		if (fork() == 0) {
			run_cmd(cmd);
		}
		wait(NULL);
		free(cmd->data);
		free(cmd);
	}

	return 0;
}

void print_prompt() {
	static i8 cwd[256];
	getcwd(cwd, 256);
	printf("[%s]$ ", cwd);
}

i8 *read_input() {
	u8 c;
	u32 len = 0;
	bool newline = false;
	while (!newline) {
		c = getchar();
		if (c == '\n') {
			buf[len] = '\0';
			printf("%c", c);
			return buf;
		} else if (c == '\b') {
			if (len > 0) {
				buf[--len] = '\0';
				printf("%c", c);
			}
		} else if (c == '\t') {
			continue;
		} else {
			if (c < ' ') {
				break;
			}
			buf[len++] = c;
			printf("%c", c);
		}
	}
	return buf;
}

static i8 whitespaces[] = " \t\r\n";

struct cmd *parse_cmd(i8 *input) {
	if (!input || !strlen(input)) {
		return NULL;
	}
	i8 **argv = malloc(10);
	i32 i_argc = 0;

	i8 *s = input;
	i8 *ends = input + strlen(input);

	// Skip whitespaces in the beginning	
	while (s < ends && strchr(whitespaces, *s) != NULL) {
		++s;
	}

	i8 *arg;
	while((arg = strsep(&s, " ")) != NULL) {
		printf("arg - %s\n", arg);
		argv[i_argc] = strdup(arg);
		++i_argc;
		if (i_argc == 10) {
			printf("too many args\n");
			return NULL;
		}
	}
	struct exec_cmd *execmd = malloc(sizeof(struct exec_cmd));
	execmd->argc = i_argc;
	argv[i_argc] = NULL;
	memcpy(execmd->argv, argv, (i_argc + 1) * 4);
	struct cmd *cmd = malloc(sizeof(struct cmd));
	cmd->type = 1;
	cmd->data = (void *)execmd;
	return cmd;
}

void run_cmd(struct cmd *cmd) {
	if (!cmd) {
		return;
	}
	if (cmd->type == 1) {
		struct exec_cmd *exec = (struct exec_cmd *)cmd->data;
		execv(exec->argv[0], exec->argv);
	}
}
