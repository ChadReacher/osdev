#include <paging.h>
#include <string.h>
#include <stdio.h>
#include <screen.h>
#include <debug.h>

page_directory_t *cur_page_dir = (page_directory_t *)0xFFFFF000;

void pagefault_handler(registers_state *regs) {
	u32 bad_address;

	__asm__ __volatile__ ("movl %%cr2, %0" : "=r"(bad_address));

	kprintf("Page Fault Exception. Error code - %x\n", regs->err_code);
	kprintf("Bad Address: %x\n", bad_address);

	while (1) {
		__asm__ __volatile__ ("hlt");
	}
}

// Return a page for a given virtual address in the current page directory
page_table_entry *get_page(virtual_address virt_addr) {
	// Get current page directory
	page_directory_t *page_dir = cur_page_dir;

	// Get page table in page directory
	page_table_t *table = (page_table_t *)(0xFFC00000 + (PAGE_DIR_INDEX((u32)virt_addr) << 12));
	
	// Get page in table
	page_table_entry *page = &table->entries[PAGE_TABLE_INDEX(virt_addr)];

	return page;
}

void map_page(void *phys_addr, void *virt_addr) {
	page_directory_entry *entry = &cur_page_dir->entries[PAGE_DIR_INDEX((u32)virt_addr)];


	if ((*entry & PAGING_FLAG_PRESENT) != PAGING_FLAG_PRESENT) {
		page_table_t *table = (page_table_t *)allocate_blocks(1);
		DEBUG("Created new table at %p\r\n", (void *)table);
		if (!table) {
			return;
		}
		*entry |= PAGING_FLAG_PRESENT;
		*entry |= PAGING_FLAG_WRITEABLE;
		*entry = ((*entry & ~0xFFFFF000) | (physical_address)table);

		page_table_t *new_page_table = (page_table_t *)(0xFFC00000 + (PAGE_DIR_INDEX((u32)virt_addr) << 12));
		memset(new_page_table, 0, sizeof(page_table_t));

		page_table_entry *page = &new_page_table->entries[PAGE_TABLE_INDEX((u32)virt_addr)];
		*page |= PAGING_FLAG_PRESENT;
		*page = ((*page & ~0xFFFFF000) | (physical_address)phys_addr);
		return;
	}


	page_table_t *table = (page_table_t *)(0xFFC00000 + (PAGE_DIR_INDEX((u32)virt_addr) << 12));

	page_table_entry *page = &table->entries[PAGE_TABLE_INDEX((u32)virt_addr)];
	*page |= PAGING_FLAG_PRESENT;	
	*page = ((*page & ~0xFFFFF000) | (physical_address)phys_addr);
}

void unmap_page(void *virt_addr) {
	page_table_entry *page = get_page((u32)virt_addr);

	*page &= ~PAGING_FLAG_PRESENT;	
	*page = ((*page & ~0xFFFFF000) | 0);

	// Flush TLB
	__asm__ __volatile__ ("movl %%cr3, %%eax" : : );
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : );
}

void paging_init() {
	register_interrupt_handler(14, pagefault_handler);

	// Identity map framebuffer
	u32 fb_size_in_bytes = SCREEN_SIZE * 4;
	u32 fb_size_in_pages = fb_size_in_bytes / PAGE_SIZE;
	if (fb_size_in_pages % PAGE_SIZE > 0) ++fb_size_in_pages;
	for (u32 i = 0, fb_start = 0xFD000000; i < fb_size_in_pages; ++i, fb_start += PAGE_SIZE) {
		map_page((void *)fb_start, (void *)fb_start);
	}

	DEBUG("%s", "Paging has been initialized\r\n");
}

void *virtual_to_physical(void *virt_addr) {
	// Get page table in page directory
	page_directory_entry *entry = &cur_page_dir->entries[PAGE_DIR_INDEX((u32)virt_addr)];
	if (!*entry) {
		return NULL;
	}

	page_table_t *table = (page_table_t *)(0xFFC00000 + (PAGE_DIR_INDEX((u32)virt_addr) << 12));
	if (!table) {
		return NULL;
	}
	
	// Get page in table
	page_table_entry *page = &table->entries[PAGE_TABLE_INDEX((u32)virt_addr)];
	if (!*page) {
		return NULL;
	}
	u32 page_frame = (u32)GET_FRAME(*page);
	u32 page_frame_offset = PAGE_FRAME_INDEX((u32)virt_addr);
	u32 phys_addr = page_frame + page_frame_offset;

	return (void *)phys_addr;
}

page_directory_t *paging_copy_page_dir(bool is_deep_copy) {
	void *new_page_dir_phys = (page_directory_t *)allocate_blocks(1);
	map_page(new_page_dir_phys, 0xE0000000);
	memset(0xE0000000, 0, 4096);

	page_directory_t *new_pd = (page_directory_t *)0xE0000000;
	page_directory_t *cur_pd = (page_directory_t *)0xFFFFF000;
	for (u32 i = 768; i < 1024; ++i) {
		new_pd->entries[i] = cur_pd->entries[i];
	}
	new_pd->entries[1023] = (u32)new_page_dir_phys | PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE;

	if (!is_deep_copy) {
		unmap_page(0xE0000000);
		return new_page_dir_phys;
	}

	// Deep copy user pages
	for (u32 i = 0; i < 768; ++i) {
		if (!cur_pd->entries[i]) {
			continue;
		}
		page_table_t *cur_table = (page_table_t *)(0xFFC00000 + (i << 12));
		page_table_t *new_table_phys = allocate_blocks(1);
		page_table_t *new_table = (page_table_t *)0xEA000000;
		map_page(new_table_phys, 0xEA000000); // Temporary mapping
		memset(0xEA000000, 0, 4096);
		for (u32 j = 0; j < 1024; ++j) {
			if (!cur_table->entries[j]) {
				continue;
			}
			// Copy the page frame's contents 
			page_table_entry cur_pte = cur_table->entries[j];
			u32 cur_page_frame = (u32)GET_FRAME(cur_pte);
			void *new_page_frame = allocate_blocks(1);
			map_page(cur_page_frame, 0xEB000000);
			map_page(new_page_frame, 0xEC000000);
			memcpy(0xEC000000, 0xEB000000, 4096);
			unmap_page(0xEC000000);
			unmap_page(0xEB000000);

			// Insert the corresponding page table entry
			page_table_entry new_pte = 0;
			if ((cur_pte & PAGING_FLAG_PRESENT) == PAGING_FLAG_PRESENT) {
				new_pte |= PAGING_FLAG_PRESENT;
			}
			if ((cur_pte & PAGING_FLAG_WRITEABLE) == PAGING_FLAG_WRITEABLE) {
				new_pte |= PAGING_FLAG_WRITEABLE;
			}
			if ((cur_pte & PAGING_FLAG_ACCESSED) == PAGING_FLAG_ACCESSED) {
				new_pte |= PAGING_FLAG_ACCESSED;
			}
			if ((cur_pte & PAGING_FLAG_DIRTY) == PAGING_FLAG_DIRTY) {
				new_pte |= PAGING_FLAG_DIRTY;
			}
			if ((cur_pte & PAGING_FLAG_USER) == PAGING_FLAG_USER) {
				new_pte |= PAGING_FLAG_USER;
			}
			new_pte = ((new_pte & ~0xFFFFF000) | (physical_address)new_page_frame);
			new_table->entries[j] = new_pte;
		}
		unmap_page(0xEA000000);

		// Insert the corresponding page directory entry
		page_directory_entry cur_pde = cur_pd->entries[i];
		page_directory_entry new_pde = 0;
		if ((cur_pde & PAGING_FLAG_PRESENT) == PAGING_FLAG_PRESENT) {
			new_pde |= PAGING_FLAG_PRESENT;
		}
		if ((cur_pde & PAGING_FLAG_WRITEABLE) == PAGING_FLAG_WRITEABLE) {
			new_pde |= PAGING_FLAG_WRITEABLE;
		}
		if ((cur_pde & PAGING_FLAG_ACCESSED) == PAGING_FLAG_ACCESSED) {
			new_pde |= PAGING_FLAG_ACCESSED;
		}
		if ((cur_pde & PAGING_FLAG_DIRTY) == PAGING_FLAG_DIRTY) {
			new_pde |= PAGING_FLAG_DIRTY;
		}
		if ((cur_pde & PAGING_FLAG_USER) == PAGING_FLAG_USER) {
			new_pde |= PAGING_FLAG_USER;
		}
		new_pde = ((new_pde & ~0xFFFFF000) | (physical_address)new_table_phys);
		new_pd->entries[i] = new_pde;
	}
	unmap_page(0xE0000000);

	return new_page_dir_phys;
}
