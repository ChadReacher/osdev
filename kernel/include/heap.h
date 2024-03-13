#ifndef HEAP_H
#define HEAP_H

#include "types.h"

#define HEAP_START			0xC0400000
#define HEAP_MAX_ADDRESS	0xCFFFFFFF
#define HEAP_INITIAL_SIZE	(48 * 1024 * 1024)  /* 48 MB */
#define HEAP_MIN_SIZE		(4 * 1024 * 1024)   /* 4 MB */

#define ALIGN(sz) ((sz / 0x10 + 1) * 0x10)

struct _heap_block {
	u32 size;
	struct _heap_block *next;
	bool free;
};

typedef struct _heap_block heap_block;

void heap_init();
void *sbrk(u32 increment);
heap_block *best_fit(u32 size);
void split_block(heap_block *block, u32 size);
void *malloc(u32 size);
void free(void *ptr);
void *realloc(void *ptr, u32 size);

#endif
