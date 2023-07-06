#include "memory.h"

void *memset(void *ptr, u32 value, size_t num) {
	u8 *c_ptr;

	c_ptr = (u8 *)ptr;
	for (size_t i = 0; i < num; ++i) {
		*c_ptr++ = (u8) value;
	}
	return ptr;
}

void *memcpy(void *dst, const void* src, size_t num) {
	u8 *dest;
	const u8 *source;

	dest = (u8 *)dst;
	source = (const u8 *)src;
	for (size_t i = 0; i < num; ++i) {
		dest[i] = source[i];
	}
	return dest;
}
