#include "unistd.h"
#include "fcntl.h"
#include "string.h"

void test(const i8 *s) {
	__asm__ __volatile__ ("int $0x80" : /* no output */ : "a"(0), "b"(s));
}

u32 read(i32 fd, const void *buf, u32 count) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(1), "b"(fd), "c"(buf), "d"(count));

	return ret;
}

u32 write(i32 fd, const void *buf, u32 count) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(2), "b"(fd), "c"(buf), "d"(count));

	return ret;
}

i32 close(i32 fd) {
	i32 ret;
	__asm__ __volatile__ ("int $0x80" : "=a"(ret) : "a"(4), "b"(fd));
	return ret;
}

i32 lseek(i32 fd, i32 offset, i32 whence) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(5), "b"(fd), "c"(offset), "d"(whence));

	return ret;
}

i32 unlink(const i8 *pathname) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(6), "b"(pathname));

	return ret;
}

void yield() {
	__asm__ __volatile__ ("int $0x80" : : "a"(7));
}

static i32 execve(const i8 *pathname, i8 *const argv[], i8 *const envp[]) {
    i32 ret;
	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(8), "b"(pathname), "c"(argv), "d"(envp));

	return ret;
}

#define PATH_LEN_MAX 256
i32 execvpe(const i8 *file, i8 *const argv[], i8 *const envp[]) {
    i8 cmd[PATH_LEN_MAX];
    memset(cmd, 0, PATH_LEN_MAX * sizeof(i8));

    if (strchr(file, '/') == NULL) {
        u32 n;
        i8 *endp;
        i8 *envpath = getenv("PATH");
        while (envpath != NULL) {
            endp = strchr(envpath, ':');
            if (endp != NULL) {
                n = endp - envpath;
            } else {
                n = strlen(envpath);
            }
            if (n + 2 + strlen(file) > PATH_LEN_MAX) {
                continue;
            }
            memcpy(cmd, envpath, n);
            cmd[n++] = '/';
            strcpy(cmd + n, file);
            if (access(cmd, F_OK) == 0) {
                break;
            }
            if (!endp) {
                return -1;
            }
            envpath = endp + 1;
        }
    } else {
        if (strlen(file) >= PATH_LEN_MAX) {
            return -1;
        }
        strcpy(cmd, file);
    }
    return execve(cmd, argv, envp);
}

i32 execv(const i8 *pathname, i8 *const argv[]) {
	return execve(pathname, argv, environ);
}

i32 fork() {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(9));

	return ret;
}

void exit(i32 exit_code) {
	__asm__ __volatile__ ("int $0x80" : : "a"(10), "b"(exit_code));
}


i32 waitpid(i32 pid, i32 *wstatus, i32 options) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(11), "b"(pid), "c"(wstatus), "d"(options));

	return ret;
}

i32 wait(i32 *wstatus) {
	return waitpid(-1, wstatus, 0);
}

i32 getpid() {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(12));

	return ret;
}

i32 dup(i32 oldfd) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(13), "b"(oldfd));

	return ret;
}

void *sbrk(u32 incr) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(14), "b"(incr));

	return (void *)ret;
}

i32 nanosleep(const struct timespec *req, struct timespec *rem) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(15), "b"(req), "c"(rem));

	return ret;
}

u32 sleep(u32 seconds) {
	struct timespec req;
	req.tv_sec = seconds;
	req.tv_nsec = 0;
	nanosleep(&req, NULL);
	return 0;
}


i8 *getcwd(i8 *buf, u32 size) {
	i8 *ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(16), "b"(buf), "c"(size));

	return ret;
}

i32 stat(const i8 *pathname, struct stat *statbuf) {
	i32 fd;
	if ((fd = open(pathname, O_RDONLY, 0)) < 0) {
		return -1;
	}
	return fstat(fd, statbuf);
}

i32 fstat(i32 fd, struct stat *statbuf) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(17), "b"(fd), "c"(statbuf));

	return ret;
}

i32 chdir(const i8 *path) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(18), "b"(path));

	return ret;
}

i32 access(const i8 *pathname, i32 mode) {
    struct stat st;

    if (stat(pathname, &st) != 0) {
        return -1;
    }
    i32 res = 0;
    if ((mode & F_OK) == F_OK) {
        res = 0;
    }
    if ((mode & R_OK) == R_OK)  {
        if ((st.st_mode & S_IRWXU) == S_IRWXU
                || (st.st_mode & S_IRUSR) == S_IRUSR
                || (st.st_mode & S_IRWXG) == S_IRWXG
                || (st.st_mode & S_IRGRP) == S_IRGRP
                || (st.st_mode & S_IRWXO) == S_IRWXO
                || (st.st_mode & S_IROTH) == S_IROTH) {
            res = 0;
        } else { 
            return -1;
        }
    }    
    if ((mode & W_OK) == W_OK)  {
        if ((st.st_mode & S_IRWXU) == S_IRWXU
                || (st.st_mode & S_IWUSR) == S_IWUSR
                || (st.st_mode & S_IRWXG) == S_IRWXG
                || (st.st_mode & S_IWGRP) == S_IWGRP
                || (st.st_mode & S_IRWXO) == S_IRWXO
                || (st.st_mode & S_IWOTH) == S_IWOTH) {
            res = 0;
        } else { 
            return -1;
        }
    }
    if ((mode & X_OK) == X_OK)  {
        if ((st.st_mode & S_IRWXU) == S_IRWXU
                || (st.st_mode & S_IXUSR) == S_IXUSR
                || (st.st_mode & S_IRWXG) == S_IRWXG
                || (st.st_mode & S_IXGRP) == S_IXGRP
                || (st.st_mode & S_IRWXO) == S_IRWXO
                || (st.st_mode & S_IXOTH) == S_IXOTH) {
            res = 0;
        } else { 
            return -1;
        }
    }
    return res;
}

i8 *getenv(const i8 *name) {
    u32 name_len = strlen(name);
    if (name == NULL || name_len == 0 || strchr(name, '=') != NULL) {
        return NULL;
    }

    u32 i = 0;
    i8 *value = NULL;
    while (environ[i] != NULL) {
        if (strncmp(name, environ[i], name_len) == 0) {
            value = environ[i] + name_len + 1;
            break;
        }
        ++i;
    }

    return value;
}

