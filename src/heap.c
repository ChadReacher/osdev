#include "heap.h"
#include "pmm.h"
#include "paging.h"
#include "stdio.h"
#include "debug.h"

void *heap_start = NULL;
void *heap_curr = NULL;
void *heap_end = NULL;
void *heap_max = NULL;

static heap_block *block_list = NULL;

void heap_init() {
	DEBUG("%s", "Heap initialization started..\r\n");
	u32 heap_blocks;
	u32 heap_phys_addr, heap_virt_addr;

	heap_start = (void *)HEAP_START;
	heap_curr = heap_start;
	heap_end = (u8 *)heap_start + HEAP_INITIAL_SIZE;
	heap_max = (void *)HEAP_MAX_ADDRESS;
	DEBUG("Heap initial size - 0x%x\r\n", HEAP_INITIAL_SIZE);
	DEBUG("Virtual heap start - %p\r\n", heap_start);
	DEBUG("Virtual heap end - %p\r\n", heap_end);
	DEBUG("Virtual heap max address - %p\r\n", heap_max);


	heap_blocks = HEAP_INITIAL_SIZE / BLOCK_SIZE;
	DEBUG("Heap consists of 0x%x heap blocks(4KB)\r\n", heap_blocks);
	void *heap_ptr = allocate_blocks(heap_blocks);
	DEBUG("Allocated %d heap blocks at physical address %p\r\n", heap_blocks, heap_ptr);
	DEBUG("Heap ends at physical address %p\r\n", (u8*)heap_ptr + HEAP_INITIAL_SIZE);

	heap_virt_addr = (u32)heap_start;
	heap_phys_addr = (u32)heap_ptr;
	DEBUG("%s", "Start with:\r\n");
	DEBUG("heap_virt_addr = %x\r\n", heap_virt_addr);
	DEBUG("heap_phys_addr = %x\r\n", heap_phys_addr);

	for (u32 i = 0; i < heap_blocks; ++i) {
		map_page((void *)heap_phys_addr, (void *)heap_virt_addr);
		heap_virt_addr += PAGE_SIZE;
		heap_phys_addr += BLOCK_SIZE;
	}
	DEBUG("%s", "Finished heap\r\n");
}

void *sbrk(u32 increment) {
	u32 old_boundary, new_boundary;

	old_boundary = (u32)heap_curr;
	new_boundary = (u32)heap_curr + increment;

	if (new_boundary <= (u32)heap_max) {
		heap_curr = (void *)new_boundary;
		return (void*)old_boundary;
	}
	return NULL;
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
void *malloc(size_t size) {
	size_t real_size;

	if (size == 0) {
		return NULL;
	}

	real_size = sizeof(heap_block) + size;
	real_size = ALIGN(real_size);

	heap_block *block;

	if (block_list) {
		heap_block* best_block = best_fit(size);
		if (best_block) {
			best_block->free = false;
			split_block(best_block, size);
			block = best_block;
		} else {
			block = sbrk(real_size);
			block->size = size;
			block->next = NULL;
			block->free = false;
			heap_block *last = block_list;
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
		// block + 1 = (u8*)block + 1 * sizeof(heap_block)
		return block + 1; 
	}
	return NULL;
}

void free(void *ptr) {
	if (!ptr) {
		return;
	}

	// Find heap block to free
	heap_block *block = (heap_block *)((u8 *)ptr - sizeof(heap_block));

	block->free = true;
	
	// Find heap block to free with its previous and next blocks
	heap_block *prev_block = NULL;
	heap_block *curr_block = block_list;
	heap_block *next_block = block_list->next;

	while (curr_block && next_block) {
		if (curr_block == block) {
			break;
		}
		prev_block = curr_block;
		curr_block = next_block;
		next_block = curr_block->next;
	}

	if (next_block && next_block->free) {
		// Merge current block with next block
		curr_block->size = curr_block->size + sizeof(heap_block) + next_block->size;
		curr_block->next = next_block->next;
	}
	if (prev_block && prev_block->free) {
		// Merge previous block with current one
		prev_block->size = prev_block->size + sizeof(heap_block) + curr_block->size;
		prev_block->next = curr_block->next;
		curr_block = prev_block;
	}

}


