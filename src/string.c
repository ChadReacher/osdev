#include "string.h"

size_t strlen(const i8 *str) {
	size_t len = 0;
	while (*str) {
		++len;
		++str;
	}
	return len;
}

i8 *strrev(i8 *str) {
	size_t sz;
	i8 temp;

	sz = strlen(str);

	for (size_t i = 0; i < sz / 2; ++i) {
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
