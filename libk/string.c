#include "string.h"
#include "heap.h"

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
