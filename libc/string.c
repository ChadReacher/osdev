#include "string.h"
#include "stdlib.h"
#include "errno.h"

void *memset(void *ptr, u32 value, u32 num) {
	u8 *c_ptr;
	u32 i;

	c_ptr = (u8 *)ptr;
	for (i = 0; i < num; ++i) {
		*c_ptr++ = (u8) value;
	}
	return ptr;
}

void *memcpy(void *dst, const void* src, u32 num) {
	u8 *dest;
	const u8 *source;
	u32 i;

	dest = (u8 *)dst;
	source = (const u8 *)src;
	for (i = 0; i < num; ++i) {
		dest[i] = source[i];
	}
	return dest;
}

u32 strlen(const i8 *str) {
	u32 len = 0;
	while (*str) {
		++len;
		++str;
	}
	return len;
}

i8 *strrev(i8 *str) {
	u32 sz, i;
	i8 temp;

	sz = strlen(str);

	for (i = 0; i < sz / 2; ++i) {
		temp = str[i];
		str[i] = str[sz - i - 1];
		str[sz - i - 1] = temp;
	}

	return str;
}

u8 strncmp(const i8 *left, const i8 *right, i32 len) {
	while (len-- && *left && *right) {
		if (*left != *right) {
			return *left - *right;
		}
		++left;
		++right;
	}
	if (len >= 0) {
		return *left - *right;
	}

	return 0;
}

i8 *strcpy(i8 *dest, const i8 *src) {
	i8 *cpy_dest = dest;
	while (*src) {
		*dest++ = *src++;
	}
	*dest = '\0';
	return cpy_dest;
}

i8 *strncpy(i8 *dest, const i8 *src, u32 n) {
	u32 i;

	if (n == 0) {
		return dest;
	}

	for (i = 0; i < n && src[i] != '\0'; ++i) {
		dest[i] = src[i];
	}

	for (; i < n; ++i) {
		dest[i] = '\0';
	}

	return dest;
}

u32 strcspn(const i8 *str1, const i8 *str2) {
	const i8 *init_str1 = str1;
	const i8 *c;

	while (*str1) {
		for (c = str2; *c; ++c) {
			if (*str1 == *c) {
				break;
			}
		}

		if (*c) {
			break;
		}

		++str1;
	}

	return str1 - init_str1;
}

i8 *strsep(i8 **str, const i8 *sep) {
	i8 *end, *s;

	if (*str == NULL) {
		return NULL;
	}

	s = *str;
	end = s + strcspn(s, sep);
	if (*end) {
		*end = 0;
		++end;
	} else {
		end = NULL;
	}

	*str = end;
	return s;
}

i32 strcmp(const i8 *str1, const i8 *str2) {
	u8 c1, c2;

	while ((c1 = *str1) == (c2 = *str2)) {
		if (c1 == '\0') {
			return 0;
		}
		++str1;
		++str2;
	}
	return c1 - c2;
}

i8 *strdup(const i8 *str) {
	u32 len = strlen(str);
	i8 *ret = malloc(len + 1);
	if (ret) {
		strncpy(ret, str, len + 1);
	}
	return ret;
}

i8 *strcat(i8 *dest, const i8 *src) {
	u32 dest_len = strlen(dest);
	u32 i;

	for (i = 0; src[i] != '\0'; ++i) {
		dest[dest_len + i] = src[i];
	}
	dest[dest_len + i] = '\0';

	return dest;
}

i8 *strchr(const i8 *s, i32 c) {
	i8 *s2;
	u32 sz, i;

	s2 = (i8 *)s;
	sz = strlen(s);
	for (i = 0; i <= sz; ++i) {
		if (*s2 == c) {
			return s2;
		}
		++s2;
	}
	return NULL;
}

i8 *strerror(i32 errnum) {
	i8 *s;
	switch (errnum) {
		case 0:
			s = "Success";
			break;
		case EPERM:
			s = "Operation not permitted";
			break;
		case ENOENT:
			s = "No such file or directory";
			break;
		case ESRCH:
			s = "No such process";
			break;
		case EINTR:
			s = "Interrupted function call";
			break;
		case EIO:
			s = "Input/Output error";
			break;
		case ENXIO:
			s = "No such device or address";
			break;
		case E2BIG:
			s = "Arg list too long";
			break;
		case ENOEXEC:
			s = "Exec format error";
			break;
		case EBADF:
			s = "Bad file descriptor";
			break;
		case ECHILD:
			s = "NO child processes";
			break;
		case EAGAIN:
			s = "Resource temporarily unavailable";
			break;
		case ENOMEM:
			s = "Not enough space";
			break;
		case EACCES:
			s = "Permission denied";
			break;
		case EFAULT:
			s = "Bad address";
			break;
		case ENOTBLK:
			s = "Block device required";
			break;
		case EBUSY:
			s = "Resouce busy";
			break;
		case EEXIST:
			s = "File exists";
			break;
		case EXDEV:
			s = "Improper link";
			break;
		case ENODEV:
			s = "No such device";
			break;
		case ENOTDIR:
			s = "Not a directory";
			break;
		case EISDIR:
			s = "Is a directory";
			break;
		case EINVAL:
			s = "Invalid argument";
			break;
		case ENFILE:
			s = "Too many open files in system";
			break;
		case EMFILE:
			s = "Too many open files";
			break;
		case ENOTTY:
			s = "Inappropriate I/O control operation";
			break;
		case ETXTBSY:
			s = "Text file busy";
			break;
		case EFBIG:
			s = "File too large";
			break;
		case ENOSPC:
			s = "No space left on device";
			break;
		case ESPIPE:
			s = "Invalid seek";
			break;
		case EROFS:
			s = "Read-only file system";
			break;
		case EMLINK:
			s = "Too many links";
			break;
		case EPIPE:
			s = "Broken pipe";
			break;
		case EDOM:
			s = "Domain error";
			break;
		case ERANGE:
			s = "Result too large";
			break;
		case EDEADLK:
			s = "Resource deadlock avoided";
			break;
		case ENAMETOOLONG:
			s = "Filename too long";
			break;
		case ENOLCK:
			s = "No locks available";
			break;
		case ENOSYS:
			s = "Function not implemented";
			break;
		case ENOTEMPTY:
			s = "Directory not empty";
			break;
		default:
			s = "Unknown error";
			break;
	}
	return s;
}
