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
