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
	u8 *c_dst;
	const u8 *c_src;

	c_dst = (u8 *)dst;
	c_src = (const u8 *)src;
	for (size_t i = 0; i < num; ++i) {
		c_dst[i] = c_src[i];
	}
	return dst;
}
