#include <heap.h>
#include <pmm.h>
#include <paging.h>
#include <panic.h>
#include <string.h>

void *heap_start = NULL;
void *heap_curr = NULL;
void *heap_end = NULL;
void *heap_max = NULL;

static struct heap_block *block_list = NULL;

static struct heap_block *best_fit(u32 size);
static void split_block(struct heap_block *block, u32 size);

void heap_init() {
	u32 heap_blocks;
	u32 heap_phys_addr, heap_virt_addr;
	void *heap_ptr;
	u32 i;
	debug("Heap initialization started..\r\n");

	heap_start = (void *)HEAP_START;
	heap_curr = heap_start;
	heap_end = (u8 *)heap_start + HEAP_INITIAL_SIZE;
	heap_max = (void *)HEAP_MAX_ADDRESS;
	debug("Heap initial size - 0x%x\r\n", HEAP_INITIAL_SIZE);
	debug("Virtual heap start - %p\r\n", heap_start);
	debug("Virtual heap end - %p\r\n", heap_end);
	debug("Virtual heap max address - %p\r\n", heap_max);


	heap_blocks = HEAP_INITIAL_SIZE / PMM_BLOCK_SIZE;
	debug("Heap consists of 0x%x heap blocks(4KB)\r\n", heap_blocks);
	heap_ptr = allocate_blocks(heap_blocks);
	debug("Allocated %d heap blocks at physical address %p\r\n", heap_blocks, heap_ptr);
	debug("Heap ends at physical address %p\r\n", (u8*)heap_ptr + HEAP_INITIAL_SIZE);

	heap_virt_addr = (u32)heap_start;
	heap_phys_addr = (u32)heap_ptr;
	debug("Start with:\r\n");
	debug("heap_virt_addr = %x\r\n", heap_virt_addr);
	debug("heap_phys_addr = %x\r\n", heap_phys_addr);

	for (i = 0; i < heap_blocks; ++i) {
		map_page((void *)heap_phys_addr, (void *)heap_virt_addr, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
		heap_virt_addr += PAGE_SIZE;
		heap_phys_addr += PMM_BLOCK_SIZE;
	}
	debug("heap virt addr - 0x%x\r\n", heap_virt_addr - PAGE_SIZE);
	debug("Finished heap\r\n");
}

void *sbrk(u32 increment) {
	u32 old_boundary, new_boundary;

	old_boundary = (u32)heap_curr;
	new_boundary = (u32)heap_curr + increment;

	if (new_boundary <= (u32)heap_max) {
		heap_curr = (void *)new_boundary;
		return (void*)old_boundary;
	} else {
		debug("%s", "Ooops, we have reached the HEAP MAX\r\n");
	}
	return NULL;
}

static struct heap_block *best_fit(u32 size) {
	struct heap_block *current = block_list;
	struct heap_block *best_current = NULL;
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

static void split_block(struct heap_block *block, u32 size) {
	if (block->size > sizeof(struct heap_block) + size) {
		struct heap_block *splited_block = (struct heap_block *)((u8*)block + sizeof(struct heap_block) + size);
		splited_block->free = 1;
		splited_block->size = block->size - size - sizeof(struct heap_block);
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
	u32 real_size;
	struct heap_block *block, *last;

	if (size == 0) {
		return NULL;
	}

	real_size = sizeof(struct heap_block) + size;
	real_size = ALIGN(real_size);


	if (block_list) {
		struct heap_block* best_block = best_fit(size);
		if (best_block) {
			best_block->free = 0;
			split_block(best_block, size);
			block = best_block;
		} else {
			block = sbrk(real_size);
			if (!block) {
				return NULL;
			}
			block->size = size;
			block->next = NULL;
			block->free = 0;
			last = block_list;
			while (last->next) {
				last = last->next;
			}
			last->next = block;
		}
	} else {
		block = sbrk(real_size);
		if (!block) {
			return NULL;
		}
		block->size = size;
		block->next = NULL;
		block->free = 0;
		block_list = block;
	}

	if (block) {
		return block + 1; 
	}
	return NULL;
}

void free(void *ptr) {
	struct heap_block *block;
	struct heap_block *prev_block;
	struct heap_block *curr_block;
	struct heap_block *next_block;
	if (!ptr) {
		return;
	}

	block = (struct heap_block *)((u8 *)ptr - sizeof(struct heap_block));

	block->free = 1;
	
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
		curr_block->size = curr_block->size + sizeof(struct heap_block) + next_block->size;
		curr_block->next = next_block->next;
	}
	if (prev_block && prev_block->free) {
		prev_block->size = prev_block->size + sizeof(struct heap_block) + curr_block->size;
		prev_block->next = curr_block->next;
		curr_block = prev_block;
	}
}

void *realloc(void *ptr, u32 size) {
	u32 real_size;
	struct heap_block *old_block, *new_block, *last;
	struct heap_block *block;
	u32 old_size;

	if (!ptr) {
		return malloc(size);
	} else if (ptr && size == 0) {
		free(ptr);
		return NULL;
	}

	old_block = (struct heap_block *)((u8 *)ptr - sizeof(struct heap_block));
	old_size = old_block->size;
	real_size = sizeof(struct heap_block) + size;
	
	block = best_fit(size);
	if (block) {
		block->free = 0;
		split_block(block, size);
		memcpy((void *)(block + 1), ptr, old_size);
	} else {
		new_block = (struct heap_block*)sbrk(real_size);
		if (!new_block) {
			return NULL;
		}
		new_block->size = size;
		new_block->next = NULL;
		new_block->free = 0;
		
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

