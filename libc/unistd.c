#include "unistd.h"
#include "fcntl.h"
#include "string.h"
#include "sys/utsname.h"
#include "sys/times.h"
#include "sys/stat.h"
#include "errno.h"

void test(const i8 *s) {
	__asm__ __volatile__ ("int $0x80" : /* no output */ : "a"(__NR_test), "b"(s));
}

u32 read(i32 fd, const void *buf, u32 count) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_read), "b"(fd), "c"(buf), "d"(count));

	return ret;
}

u32 write(i32 fd, const void *buf, u32 count) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_write), "b"(fd), "c"(buf), "d"(count));

	return ret;
}

i32 close(i32 fd) {
	i32 ret;
	__asm__ __volatile__ ("int $0x80" : "=a"(ret) : "a"(__NR_close), "b"(fd));
	return ret;
}

i32 lseek(i32 fd, i32 offset, i32 whence) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_lseek), "b"(fd), "c"(offset), "d"(whence));

	return ret;
}

i32 unlink(const i8 *pathname) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_unlink), "b"(pathname));

	return ret;
}

void yield() {
	__asm__ __volatile__ ("int $0x80" : : "a"(__NR_yield));
}

static i32 execve(const i8 *pathname, i8 *const argv[], i8 *const envp[]) {
    i32 ret;
	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_execve), "b"(pathname), "c"(argv), "d"(envp));

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
			: "a"(__NR_fork));

	return ret;
}

void _exit(i32 exit_code) {
	__asm__ __volatile__ ("int $0x80" : : "a"(__NR_exit), "b"(exit_code));
}


i32 waitpid(i32 pid, i32 *stat_loc, i32 options) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_waitpid), "b"(pid), "c"(stat_loc), "d"(options));

	return ret;
}

i32 wait(i32 *stat_loc) {
	return waitpid(-1, stat_loc, 0);
}

i32 getpid() {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_getpid));

	return ret;
}

i32 dup(i32 oldfd) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_dup), "b"(oldfd));

	return ret;
}

void *sbrk(u32 incr) {
	u32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_sbrk), "b"(incr));

	return (void *)ret;
}

i32 nanosleep(const struct timespec *req, struct timespec *rem) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_nanosleep), "b"(req), "c"(rem));

	return ret;
}

u32 sleep(u32 secs) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_sleep), "b"(secs));

	return ret;
}


i8 *getcwd(i8 *buf, u32 size) {
	i8 *ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_getcwd), "b"(buf), "c"(size));

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
			: "a"(__NR_fstat), "b"(fd), "c"(statbuf));

	return ret;
}

i32 chdir(const i8 *path) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_chdir), "b"(path));

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

u32 alarm(u32 secs) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_alarm), "b"(secs));

	return ret;
}

i32 getppid() {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_getppid));

	return ret;
}

u16 getuid() {

	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_getuid));

	return ret;
}

u16 geteuid() {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_geteuid));

	return ret;
}

u8 getgid() {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_getgid));

	return ret;
}

u8 getegid() {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_getegid));

	return ret;
}

i32 setuid() {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_setuid));

	return ret;
}

i32 setgid() {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_setgid));

	return ret;
}

i32 getpgrp() {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_getpgrp));

	return ret;
}


i32 setsid() {	
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_setsid));

	return ret;
}

i32 setpgid(i32 pid, i32 pgid) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_setpgid), "b"(pid), "c"(pgid));

	return ret;
}

i32 uname(utsname *name) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_uname), "b"(name));

	return ret;
}

i32 time(i32 *tloc) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_time), "b"(tloc));

	return ret;
}

i32 times(tms *buffer) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(__NR_times), "b"(buffer));

	return ret;
}

