#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "stdio.h"

static heap_block *block_list = NULL;

i8 *itoa(i32 value, i8 *str, i32 base) {
	i32 rem;
	u32 idx;
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
	u32 idx;

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

heap_block *best_fit(u32 size) {
	heap_block *current = block_list;
	heap_block *best_current = NULL;
	while (current) {
		if (current->free && current->size >= size) {
			if (best_current == NULL || current->size < best_current->size) {
				best_current = current;
			}
		}
		current = current->next;
	}
	return best_current;
}

void split_block(heap_block *block, u32 size) {
	if (block->size > sizeof(heap_block) + size) {
		heap_block *splited_block = (heap_block *)((u8*)block + sizeof(heap_block) + size);
		splited_block->free = true;
		splited_block->size = block->size - size - sizeof(heap_block);
		splited_block->next = block->next;
		block->size = size;
		block->next = splited_block;
	}
}

/* 
 * Allocate 'size' bytes
 * It allocates a header with needed number of bytes
 * So first it goes a header, than after the header
 * real allocated space goes.
 */
void *malloc(u32 size) {
	heap_block *block, *best_block, *last;
	u32 real_size;

	if (size == 0) {
		return NULL;
	}

	real_size = sizeof(heap_block) + size;
	real_size = ALIGN(real_size);

	if (block_list) {
		best_block = best_fit(size);
		if (best_block) {
			best_block->free = false;
			split_block(best_block, size);
			block = best_block;
		} else {
			block = sbrk(real_size);
			block->size = size;
			block->next = NULL;
			block->free = false;
			last = block_list;
			while (last->next) {
				last = last->next;
			}
			last->next = block;
		}
	} else {
		block = sbrk(real_size);
		block->size = size;
		block->next = NULL;
		block->free = false;
		block_list = block;
	}

	if (block) {
		return block + 1; 
	}
	return NULL;
}

void free(void *ptr) {
	heap_block *block, *prev_block, *curr_block, *next_block;

	if (!ptr) {
		return;
	}

	block = (heap_block *)((u8 *)ptr - sizeof(heap_block));

	block->free = true;
	
	prev_block = NULL;
	curr_block = block_list;
	next_block = block_list->next;

	while (curr_block && next_block) {
		if (curr_block == block) {
			break;
		}
		prev_block = curr_block;
		curr_block = next_block;
		next_block = curr_block->next;
	}

	if (next_block && next_block->free) {
		curr_block->size = curr_block->size + sizeof(heap_block) + next_block->size;
		curr_block->next = next_block->next;
	}
	if (prev_block && prev_block->free) {
		prev_block->size = prev_block->size + sizeof(heap_block) + curr_block->size;
		prev_block->next = curr_block->next;
		curr_block = prev_block;
	}
}

void *realloc(void *ptr, u32 size) {
	u32 old_size, real_size;
	heap_block *old_block, *block, *new_block, *last;

	if (!ptr) {
		return malloc(size);
	} else if (ptr && size == 0) {
		free(ptr);
		return NULL;
	}

	old_block = (heap_block *)((u8 *)ptr - sizeof(heap_block));
	old_size = old_block->size;
	real_size = sizeof(heap_block) + size;
	
	block = best_fit(size);
	if (block) {
		block->free = false;
		split_block(block, size);
		memcpy((void *)(block + 1), ptr, old_size);
	} else {
		new_block = (heap_block*)sbrk(real_size);
		new_block->size = size;
		new_block->next = NULL;
		new_block->free = false;
		
		memcpy((void *)(new_block + 1), ptr, old_size);

		last = block_list;
		while (last->next) {
			last = last->next;
		}
		last->next = new_block;

		block = new_block;
	}

	free(ptr);


	return block + 1;
}
