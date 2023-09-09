#ifndef STDLIB_H
#define STDLIB_H

#include "types.h"

#define ALIGN(sz) ((sz / 0x10 + 1) * 0x10)

i8 *itoa(i32 value, i8 *str, i32 base);
i8 *utoa(u32 value, i8 *str, u32 base);
u32 atoi(const i8 *str);

void *malloc(u32 size);
void free(void *ptr);
void *realloc(void *ptr, u32 size);

struct _heap_block {
	u32 size;
	struct _heap_block *next;
	bool free;
};

typedef struct _heap_block heap_block;


#endif
