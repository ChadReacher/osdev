#include "memory.h"

void *memset(void *ptr, u32 value, u32 num) {
	u8 *c_ptr;

	c_ptr = (u8 *)ptr;
	for (u32 i = 0; i < num; ++i) {
		*c_ptr++ = (u8) value;
	}
	return ptr;
}

void *memcpy(void *dst, const void* src, u32 num) {
	u8 *dest;
	const u8 *source;

	dest = (u8 *)dst;
	source = (const u8 *)src;
	for (u32 i = 0; i < num; ++i) {
		dest[i] = source[i];
	}
	return dest;
}
