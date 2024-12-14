#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>

#define BUFSZ 1024
#define NR_JOBS 20

enum exec_mode {
	EXEC_FOREGROUND,
	EXEC_BACKGROUND,
	EXEC_PIPELINE
};

enum cmd_type {
	CMD_CD,
	CMD_EXIT,
	CMD_JOBS,
	CMD_FG,
	CMD_BG,
	CMD_EXTERNAL
};

enum proc_status {
	RUNNING,
	DONE,
	SUSPENDED,
	CONTINUED,
	TERMINATED
};

struct process {
	char *command;
	int argc;
	char **argv;
	char *ipath;
	char *opath;
	pid_t pid;
	int type;
	int status;
	struct process *next;
};

struct job {
	int id;
	struct process *root;
	char *command;
	pid_t pgid;
	int mode;
};

struct job *job_table[NR_JOBS + 1];
char cwd[256];
char *status_str[] = {
	"running",
	"done",
	"suspended",
	"continued",
	"terminated"
};

char *strtrim(char *line) {
	char *start = line;
	char *end = line + strlen(line) - 1;

	while (*start == ' ') {
		++start;
	}
	while (*end == ' ') {
		--end;
	}
	*(end + 1) = '\0';
	return start;
}

int command_type(char *cmd) {
	if (strcmp(cmd, "cd") == 0) {
		return CMD_CD;
	} else if (strcmp(cmd, "exit") == 0) {
		return CMD_EXIT;
	} else if (strcmp(cmd, "jobs") == 0) {
		return CMD_JOBS;
	} else if (strcmp(cmd, "fg") == 0) {
		return CMD_FG;
	} else if (strcmp(cmd, "bg") == 0) {
		return CMD_BG;
	} else {
		return CMD_EXTERNAL;
	}
}


int next_job_id() {
	int i;
	
	for (i = 1; i < NR_JOBS; ++i) {
		if (!job_table[i]) {
			return i;
		}
	}
	return -1;
}

int insert_job(struct job *job) {
	int id = next_job_id();
	if (id < 0) {
		return -1;
	}
	job->id = id;
	job_table[id] = job;
	return id;
}

#define FILTER_REMAINING 0
int get_proc_count(int jobid, int filter) {
	struct process *p;
	int c = 0;

	for (p = job_table[jobid]->root; p != NULL; p = p->next) {
		if (filter == FILTER_REMAINING && p->status != DONE) {
			++c;
		}
	}
	return c;
}

int set_job_status(int jobid, int status) {
	struct process *p;
	if (jobid > NR_JOBS || job_table[jobid] == NULL){ 
		return -1;
	}
	for (p = job_table[jobid]->root; p != NULL; p = p->next) { 
		if (p->status != DONE) {
			p->status = status;
		}
	}
	return 0;
}

int set_process_status(int pid, int status) {
	struct process *p;
	int i;

	for (i = 1; i < NR_JOBS; ++i) {
		if (!job_table[i]) {
			continue;
		}
		for (p = job_table[i]->root; p != NULL; p = p->next) {
			if (p->pid == pid) {
				p->status = status;
				return 0;
			}
		}
	}
	return -1;
}


void print_processes_of_job(int id) {
	struct process *p;

	printf("[%d]", id);
	for (p = job_table[id]->root; p != NULL; p = p->next) {
		printf(" %d", p->pid);
	}
	printf("\n");
}

void print_job_status(int id) {
	struct process *p;
	
	printf("[%d]", id);
	for (p = job_table[id]->root; p != NULL; p = p->next) { 
		printf("\t%d\t%s\t%s", p->pid, status_str[p->status], p->command);
		if (p->next != NULL) {
			printf("|");
		}
		printf("\n");
	}
}

void release_job(int jobid) {
	int i;
	struct process *p, *tmp;
	struct job *job = job_table[jobid];
	for (p = job->root; p != NULL;) {
		tmp = p->next;
		free(p->command);
		for (i = 0; i < p->argc; ++i) {
			free(p->argv[i]);
		}
		free(p->argv);
		free(p->ipath);
		free(p->opath);
		free(p);
		p = tmp;
	}
	free(job->command);
	free(job);
}

void remove_job(int jobid) {
	release_job(jobid);
	job_table[jobid] = NULL;
}

int wait_for_pid(int pid) {
	int status = 0;
	waitpid(pid, &status, WUNTRACED);
	if (WIFEXITED(status)) {
		set_process_status(pid, DONE);
	} else if (WIFSIGNALED(status)) {
		set_process_status(pid, TERMINATED);
	} else if (WSTOPSIG(status)) {
		status = -1;
		set_process_status(pid, SUSPENDED);
	}
	return status;
}

int wait_for_job(int id) {
	int status = 0;
	int p_count = get_proc_count(id, FILTER_REMAINING);
	int wait_pid = -1, w_count = 0;

	do {
		wait_pid = waitpid(-job_table[id]->pgid, &status, WUNTRACED);
		++w_count;
		if (WIFEXITED(status)) {
			set_process_status(wait_pid, DONE);
		} else if (WIFSIGNALED(status)) {
			set_process_status(wait_pid, TERMINATED);
		} else if (WIFSTOPPED(status)) {
			status = -1;
			set_process_status(wait_pid, SUSPENDED);
			if (w_count == p_count) {
				print_job_status(id);
			}
		}
	} while (w_count < p_count);
	return status;
}

struct job *get_job_by_id(int id) {
	if (id > NR_JOBS) {
		return NULL;
	}
	return job_table[id];
}

int get_pgid_by_job_id(int jobid) {
	struct job *job = get_job_by_id(jobid);
	if (!job) {
		return -1;
	}
	return job->pgid;
}

int get_job_id_by_pid(int pid) {
	int i;
	struct process *p;

	for (i = 1; i < NR_JOBS; ++i) {
		if (job_table[i]) {
			for (p = job_table[i]->root; p != NULL; p = p->next) {
				if (p->pid == pid) {
					return i;
				}
			}
		}
	}
	return -1;
}

void sigint_handler(int sig) {
	(void)sig;
	printf("\n");
}

void builtin_cd(int argc, char **argv) {
	int err;
	(void)argc;

	err = chdir(argv[1]);
	if (err) {
		perror("sh: cd failed");
		return;
	}
	getcwd(cwd, 256);
	
}

void builtin_exit() {
	printf("Exiting...\n");
	_exit(0);
}

void builtin_jobs() {
	int i;

	for (i = 1; i < NR_JOBS; ++i) {
		if (job_table[i]) {
			print_job_status(i);
		}
	}
}

void builtin_fg(int argc, char **argv) {
	int pid, job_id = -1;
	if (argc < 2) {
		printf("\tusage: fg pid\n");
		return;
	}
	if (argv[1][0] == '%') {
		job_id = atoi(argv[1] + 1);
		pid = get_pgid_by_job_id(job_id);
		if (pid < 0) {
			printf("sh: fg %s: no such job\n", argv[1]);
			return;
		}
	} else {
		pid = atoi(argv[1]);
	}
	if (kill(-pid, SIGCONT) < 0) {
		printf("sh: fg %d: job not found\n", pid);
	}
	tcsetpgrp(0, pid);
	if (job_id >= 0) {
		set_job_status(job_id, CONTINUED);
		print_job_status(job_id);
		if (wait_for_job(job_id) >= 0) {
			remove_job(job_id);
		}
	} else {
		wait_for_pid(pid);
	}
	tcsetpgrp(0, getpid());
}

void builtin_bg(int argc, char **argv) {
	int pid, job_id = -1;
	if (argc < 2) {
		printf("\tusage: bg pid\n");
		return;
	}
	if (argv[1][0] == '%') {
		job_id = atoi(argv[1] + 1);
		pid = get_pgid_by_job_id(job_id);
		if (pid < 0) {
			printf("sh: bg %s: no such job\n", argv[1]);
			return;
		}
	} else {
		pid = atoi(argv[1]);
	}
	if (kill(-pid, SIGCONT) < 0) {
		printf("sh: bg %d: job not found\n", pid);
	}
	if (job_id >= 0) {
		print_job_status(job_id);
	}
}


void exec_builtin(struct process *p) {
	switch (p->type) {
		case CMD_CD:
			builtin_cd(p->argc, p->argv);
			break;
		case CMD_EXIT:
			builtin_exit();
			break;
		case CMD_JOBS:
			builtin_jobs();
			break;
		case CMD_FG:
			builtin_fg(p->argc, p->argv);
			break;
		case CMD_BG:
			builtin_bg(p->argc, p->argv);
			break;
		default:
			break;
	}
}

int run_proc(struct job *job, struct process *p, int ifd, int ofd, int mode) {
	sigaction_t sigdfl_act = { SIG_DFL, 0, 0 };
	pid_t childpid;
	int status = 0;

	sigemptyset(&sigdfl_act.sa_mask);
	p->status = RUNNING;
	if (p->type != CMD_EXTERNAL) {
		exec_builtin(p);
		return 0;
	}


	childpid = fork();
	if (childpid < 0) {
		return -1;
	} else if (childpid == 0) {
		sigaction(SIGINT, &sigdfl_act, NULL);
		sigaction(SIGQUIT, &sigdfl_act, NULL);
		sigaction(SIGTSTP, &sigdfl_act, NULL);
		sigaction(SIGTTIN, &sigdfl_act, NULL);
		sigaction(SIGTTOU, &sigdfl_act, NULL);
		sigaction(SIGCHLD, &sigdfl_act, NULL);

		p->pid = getpid();
		if (job->pgid > 0) {
			setpgid(0, job->pgid);
		} else {
			job->pgid = p->pid;
			setpgid(0, job->pgid);
		}

		if (ifd != 0) {
			dup2(ifd, 0);
			close(ifd);
		}

		if (ofd != 1) {
			dup2(ofd, 1);
			close(ofd);
		}

		if (execvp(p->argv[0], p->argv) < 0) {
			perror("sh failed");
			_exit(0);
		}
		_exit(0);
	} else {
		p->pid = childpid;
		if (job->pgid > 0) {
			setpgid(childpid, job->pgid);
		} else {
			job->pgid = p->pid;
			setpgid(childpid, job->pgid);
		}

		if (mode == EXEC_FOREGROUND) {
			tcsetpgrp(0, job->pgid);
			status = wait_for_job(job->id);
			tcsetpgrp(0, getpid());
		}
	}
	return status;
}


int is_job_completed(int id) {
	struct process *p;

	if (id > NR_JOBS || job_table[id] == NULL) {
		return 0;
	}
	for (p = job_table[id]->root; p != NULL; p = p->next) {
		if (p->status != DONE) {
			return 0;
		}
	}
	return 1;
}

void check_zombie() {
	int status, pid, jobid;
	while ((pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0) {
		if (WIFEXITED(status)) {
			set_process_status(pid, DONE);
		} else if (WIFSTOPPED(status)) {
			set_process_status(pid, SUSPENDED);
		}
		jobid = get_job_id_by_pid(pid);
		if (jobid > 0 && is_job_completed(jobid)) {
			print_job_status(jobid);
			remove_job(jobid);
		}
	}
}

void run_job(struct job *job) {
	struct process *p;
	int ifd = 0, status, job_id;
	int fd[2];

	check_zombie();
	if (job->root->type == CMD_EXTERNAL) {
		job_id = insert_job(job);
	}

	for (p = job->root; p != NULL; p = p->next) {
		if (p == job->root && p->ipath != NULL) {
			ifd = open(p->ipath, O_RDONLY, 0);
			if (ifd < 0) {
				printf("sh: no such file or directory: %s\n", p->ipath);
				remove_job(job_id);
				return;
			}
		}
		if (p->next != NULL) {
			pipe(fd);
			status = run_proc(job, p, ifd, fd[1], EXEC_PIPELINE);
			close(fd[1]);
			ifd = fd[0];
		} else {
			int ofd = 1;
			if (p->opath != NULL) {
				ofd = open(p->opath, O_CREAT|O_WRONLY|O_TRUNC, 0);
				if (ofd < 0) {
					ofd = 1;
				}
			}
			status = run_proc(job, p, ifd, ofd, job->mode);
			if (ofd != 1) {
				close(ofd);
			}
		}
	}

	if (job->root->type == CMD_EXTERNAL) {
		if (status >= 0 && job->mode == EXEC_FOREGROUND) {
			remove_job(job_id);
		} else if (job->mode == EXEC_BACKGROUND) {
			print_processes_of_job(job_id);
		}
	}
}

struct process *parse_cmd(char *cmd) {
	char **tokens;
	int count = 0, i, argc;
	char *s = strdup(cmd), *arg;
	char *ss = s;
	char *ipath = NULL, *opath = NULL;
	struct process *p;
	
	tokens = malloc(sizeof(char *) * 10);
	for (i = 0; i < 10; ++i) {
		tokens[i] = NULL;
	}
	while ((arg = strsep(&s, " ")) != NULL) {
		if (strcmp(arg, "") == 0) {
			continue;
		}
		tokens[count++] = strdup(arg);
		if (count == 10) {
			break;
		}
	}
	i = 0;
	while (i < count) {
		if (tokens[i][0] == '<' || tokens[i][0] == '>') {
			break;
		}
		++i;
	}
	argc = i;

	for (; i < count; ++i) {
		if (tokens[i][0] == '<') {
			if (strlen(tokens[i]) == 1) {
				ipath = malloc(sizeof(char) * (strlen(tokens[i + 1]) + 1));
				strcpy(ipath, tokens[i + 1]);
				++i;
			} else {
				ipath = malloc(sizeof(char) * (strlen(tokens[i]) + 1));
				strcpy(ipath, tokens[i] + 1);
			}
		} else if (tokens[i][0] == '>') {
			if (strlen(tokens[i]) == 1) {
				opath = malloc(sizeof(char) * (strlen(tokens[i + 1]) + 1));
				strcpy(opath, tokens[i + 1]);
				++i;
			} else {
				opath = malloc(sizeof(char) * (strlen(tokens[i]) + 1));
				strcpy(opath, tokens[i] + 1);
			}
		} else {
			break;
		}
	}
	for (i = argc; i < count; ++i) {
		tokens[i] = NULL;
	}
	p = malloc(sizeof(struct process));
	p->command = cmd;
	p->argc = argc;
	p->argv = tokens;
	p->ipath = ipath;
	p->opath = opath;
	p->pid = -1;
	p->type = command_type(tokens[0]);
	p->status = 0;
	p->next = NULL;
	free(ss);
	return p;
}

struct job *parse_line(char *line) {
	struct process *root = NULL, *proc = NULL, *new_proc;
	int mode = EXEC_FOREGROUND, cmd_len = 0;
	char *cmd, *cursor, *c;
	struct job *job;

	cursor = line;
	c = line;
	if (line[strlen(line) - 1] == '&') {
		mode = EXEC_BACKGROUND;
		line[strlen(line) - 1] = '\0';
	}
	while (1) {
		if (*c == '\0' || *c == '|') {
			cmd = malloc(sizeof(char) * (cmd_len + 1));
			strncpy(cmd, cursor, cmd_len);
			cmd[cmd_len] = '\0';

			new_proc = parse_cmd(cmd);
			if (!root) {
				root = new_proc;
			} else {
				proc->next = new_proc;
			}
			proc = new_proc;

			if (*c == '\0') {
				break;
			}
			cursor = c;
			while (*(++cursor) == ' ');
			c = cursor;
			cmd_len = 0;
			continue;
		} else {
			++cmd_len;
			++c;
		}
	}
	job = malloc(sizeof(struct job));
	job->id = 0;
	job->root = root;
	job->command = strdup(line);
	job->pgid = -1;
	job->mode = mode;
	return job;
}

char buf[BUFSZ];

char *read_line() {
	int c;
	int pos = 0;

	memset(buf, 0, BUFSZ);
	while (1) {
		c = getchar();
		if (c == '\n') {
			buf[pos] = '\0';
			return buf;
		} else {
			buf[pos++] = c;
		}
	}
}

void print_prompt() {
	printf("root@localhost:%s# ", cwd);
}

void loop() {
	char *line;
	struct job *job;

	while (1) {
		print_prompt();
		line = read_line();
		if (strlen(line) == 0) {
			check_zombie();
			continue;
		}
		job = parse_line(line);
		run_job(job);
	}
}

void init() {
	pid_t pid;
	sigaction_t sigign_act = { SIG_IGN, 0, 0 };
	sigaction_t sigint_act = { sigint_handler, 0, 0 };

	sigemptyset(&sigign_act.sa_mask);
	sigemptyset(&sigint_act.sa_mask);
	sigaction(SIGINT, &sigint_act, NULL);

	sigaction(SIGQUIT, &sigign_act, NULL);
	sigaction(SIGTSTP, &sigign_act, NULL);
	sigaction(SIGTTIN, &sigign_act, NULL);

	pid = getpid();
	setpgid(pid, pid);
	tcsetpgrp(0, pid);

	getcwd(cwd, 256);

	printf("\033[2J\033[;H");
}

int main() {
	init();
	loop();
	return 0;
}
