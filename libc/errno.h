#ifndef ERRNO_H
#define ERRNO_H

#include <sys/types.h>

extern i32 errno;

#define EPERM			1
#define ENOENT			1
#define ESRCH			1
#define EINTR			1
#define EIO				1
#define ENXIO			1
#define E2BIG			1
#define ENOEXEC			1
#define EBADF			1
#define ECHILD			1
#define EAGAIN			1
#define ENOMEM			1
#define EACCES			1
#define EFAULT			1
#define ENOTBLK			1
#define EBUSY			1
#define EEXIST			1
#define EXDEV			1
#define ENODEV			1
#define ENOTDIR			1
#define EISDIR			1
#define EINVAL			1
#define ENFILE			1
#define EMFILE			1
#define ENOTTY			1
#define ETXTBSY			1
#define EFBIG			1
#define ENOSPC			1
#define ESPIPE			1
#define EROFS			1
#define EMLINK			1
#define EPIPE			1
#define EDOM			1
#define ERANGE			1
#define EDEADLK			1
#define ENAMETOOLONG	1
#define ENOLCK			1
#define ENOSYS			1
#define ENOTEMPTY		1

#endif
