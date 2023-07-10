#include "k_stdlib.h"
#include "string.h"
#include "ctype.h"

i8 *itoa(i32 value, i8 *str, i32 base) {
	i32 rem;
	size_t idx;
	bool negative;

	idx = 0;
	negative = false;

	if (value == 0) {
		str[idx++] = '0';
		return str;
	}

	if (value < 0 && base == 10) {
		negative = true;
		value = -value;
	}

	while (value) {
		rem = value % base;
		str[idx++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
		value /= base;
	}

	if (negative) {
		str[idx] = '-';
	}

	strrev(str);

	return str;
}

i8 *utoa(u32 value, i8 *str, u32 base) {
	i32 rem;
	size_t idx;

	idx = 0;
	if (value == 0) {
		str[idx++] = '0';
		return str;
	}

	while (value) {
		rem = value % base;
		str[idx++] = (rem > 9) ? (rem - 10) + 'A' : rem + '0';
		value /= base;
	}

	strrev(str);

	return str;
}

u32 atoi(const i8 *str) {
	u32 i = 0;
	while (isdigit(*str)) {
		i = i * 10  + (*str - '0');
		++str;
	}
	return i;
}
