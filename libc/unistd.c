#include "unistd.h"
#include "fcntl.h"

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

i32 execve(const i8 *pathname, i8 *const argv[], i8 *const envp[]) {
	i32 ret;

	__asm__ __volatile__ ("int $0x80" 
			: "=a"(ret) 
			: "a"(8), "b"(pathname), "c"(argv), "d"(envp));

	return ret;
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
