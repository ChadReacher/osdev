#include <paging.h>
#include <string.h>
#include <panic.h>
#include <pmm.h>
#include <process.h>
#include <stdio.h>

extern struct proc *current_process;

page_directory_t *cur_page_dir = (page_directory_t *)0xFFFFF000;

void pagefault_handler(struct registers_state *regs) {
	u32 bad_address, err_code;
	i8 not_present, rw, us;

	__asm__ volatile ("movl %%cr2, %0" : "=r"(bad_address));
	err_code = regs->err_code;

	debug("Page Fault Exception. Bad Address: 0x%x. Error code: %d\r\n", bad_address, err_code);
	kprintf("page fault\r\n");

	not_present = err_code & 0x1;
	rw = err_code & 0x2;
	us = err_code & 0x4;
	
	if (!not_present && rw && us) {
		void *new_heap_page;
		debug("%s", "User heap\r\n");
		/* Fault due to user heap expansion */
		new_heap_page = allocate_blocks(1);
		map_page(new_heap_page, (void *)bad_address, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE | PAGING_FLAG_USER);
	} else {
		debug("%s", "Not user heap\r\n");
		while (1) {
			__asm__ volatile ("cli");
			__asm__ volatile ("hlt");
		}
	}
}

/* Return a page for a given virtual address in the current page directory */
page_table_entry *get_page(virtual_address virt_addr) {
	/* Get page table in page directory */
	page_table_t *table = (page_table_t *)(0xFFC00000 + (PAGE_DIR_INDEX((u32)virt_addr) << 12));
	
	/* Get page in table */
	page_table_entry *page = &table->entries[PAGE_TABLE_INDEX(virt_addr)];

	return page;
}

void map_page(void *phys_addr, void *virt_addr, u32 flags) {
	page_table_t *table;
	page_table_entry *page;
	page_directory_entry *entry = &cur_page_dir->entries[PAGE_DIR_INDEX((u32)virt_addr)];


	if ((*entry & PAGING_FLAG_PRESENT) != PAGING_FLAG_PRESENT) {
		page_table_t *new_page_table;
		table = (page_table_t *)allocate_blocks(1);
		debug("Created new table at %p\r\n", (void *)table);
		if (!table) {
			return;
		}
		*entry |= PAGING_FLAG_PRESENT;
		*entry |= PAGING_FLAG_WRITEABLE;
		*entry = ((*entry & ~0xFFFFF000) | (physical_address)table);

		new_page_table = (page_table_t *)(0xFFC00000 + (PAGE_DIR_INDEX((u32)virt_addr) << 12));
		memset(new_page_table, 0, sizeof(page_table_t));

		page = &new_page_table->entries[PAGE_TABLE_INDEX((u32)virt_addr)];
		*page |= flags;
		*page = ((*page & ~0xFFFFF000) | (physical_address)phys_addr);
		return;
	}


	table = (page_table_t *)(0xFFC00000 + (PAGE_DIR_INDEX((u32)virt_addr) << 12));

	page = &table->entries[PAGE_TABLE_INDEX((u32)virt_addr)];
	*page |= flags;	
	*page = ((*page & ~0xFFFFF000) | (physical_address)phys_addr);
}

void unmap_page(void *virt_addr) {
	page_table_entry *page = get_page((u32)virt_addr);

	*page &= ~PAGING_FLAG_PRESENT;	
	*page = ((*page & ~0xFFFFF000) | 0);

	/* Flush TLB */
	__asm__ volatile ("movl %%cr3, %%eax" : : );
	__asm__ volatile ("movl %%eax, %%cr3" : : );
}

void paging_init() {
	register_interrupt_handler(14, pagefault_handler);

	debug("Paging has been initialized\r\n");
}

void *virtual_to_physical(void *virt_addr) {
	page_directory_entry *entry;
	page_table_t *table;
	page_table_entry *page;
	u32 page_frame, page_frame_offset, phys_addr;
	/* Get page table in page directory */
	entry = &cur_page_dir->entries[PAGE_DIR_INDEX((u32)virt_addr)];
	if (!*entry) {
		return NULL;
	}

	table = (page_table_t *)(0xFFC00000 + (PAGE_DIR_INDEX((u32)virt_addr) << 12));
	if (!table) {
		return NULL;
	}
	
	/* Get page in table */
	page = &table->entries[PAGE_TABLE_INDEX((u32)virt_addr)];
	if (!*page) {
		return NULL;
	}
	page_frame = (u32)GET_FRAME(*page);
	page_frame_offset = PAGE_FRAME_INDEX((u32)virt_addr);
	phys_addr = page_frame + page_frame_offset;

	return (void *)phys_addr;
}

void free_user_image() {
	u32 i, j;
	page_directory_t *page_dir;
	void *page_dir_phys;
	page_directory_entry pde;
	page_table_t *table;
	page_table_entry pte;
	void *table_phys;
	void *page_frame;

	page_dir_phys = (void *)current_process->directory;
	map_page(page_dir_phys, (void *)0xE0000000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
	page_dir = (page_directory_t *)0xE0000000;
	for (i = 0; i < 768; ++i) {
		if (!page_dir->entries[i]) {
			continue;
		}
		pde = page_dir->entries[i];
		table_phys = (void *)GET_FRAME(pde);
		map_page(table_phys, (void *)0xEA000000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
		table = (page_table_t *)0xEA000000;
		for (j = 0; j < 1024; ++j) {
			if (!table->entries[j]) {
				continue;
			}
			pte = table->entries[j];
			page_frame = (void *)GET_FRAME(pte);
			free_blocks(page_frame, 1);
		}
		memset(table, 0, 4096);
		unmap_page((void *)0xEA000000);
		free_blocks(table_phys, 1);
		page_dir->entries[i] = 0;
	}
	unmap_page((void *)0xE0000000);

	/* Flush TLB */
	__asm__ volatile ("movl %%cr3, %%eax" : : );
	__asm__ volatile ("movl %%eax, %%cr3" : : );
}

page_directory_t *paging_copy_page_dir(bool is_deep_copy) {
	u32 i, j;
	page_directory_t *new_pd, *cur_pd;
	void *new_page_dir_phys = (page_directory_t *)allocate_blocks(1);

	if (new_page_dir_phys == NULL) {
		return NULL;
	}
	map_page(new_page_dir_phys, (void *)0xE0000000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
	memset((void *)0xE0000000, 0, 4096);

	new_pd = (page_directory_t *)0xE0000000;
	cur_pd = (page_directory_t *)0xFFFFF000;
	for (i = 768; i < 1024; ++i) {
		new_pd->entries[i] = cur_pd->entries[i];
	}
	new_pd->entries[1023] = (u32)new_page_dir_phys | PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE;

	if (!is_deep_copy) {
		unmap_page((void *)0xE0000000);
		return new_page_dir_phys;
	}

	/* Deep copy user pages */
	for (i = 0; i < 768; ++i) {
		page_directory_entry cur_pde, new_pde; 
		page_table_t *cur_table, *new_table_phys, *new_table;

		if (!cur_pd->entries[i]) {
			continue;
		}
		cur_table = (page_table_t *)(0xFFC00000 + (i << 12));
		new_table_phys = allocate_blocks(1);
		if (new_table_phys == NULL) {
			return NULL;
		}
		new_table = (page_table_t *)0xEA000000;
		map_page(new_table_phys, (void *)0xEA000000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE); /* Temporary mapping */
		memset((void *)0xEA000000, 0, 4096);
		for (j = 0; j < 1024; ++j) {
			page_table_entry cur_pte, new_pte;
			u32 cur_page_frame;
			void *new_page_frame;

			if (!cur_table->entries[j]) {
				continue;
			}
			/* Copy the page frame's contents  */
			cur_pte = cur_table->entries[j];
			cur_page_frame = (u32)GET_FRAME(cur_pte);
			new_page_frame = allocate_blocks(1);
			if (new_page_frame == NULL) {
				return NULL;
			}
			map_page((void *)cur_page_frame, (void *)0xEB000000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
			map_page(new_page_frame, (void *)0xEC000000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
			memcpy((void *)0xEC000000, (void *)0xEB000000, 4096);
			unmap_page((void *)0xEC000000);
			unmap_page((void *)0xEB000000);

			/* Insert the corresponding page table entry */
			new_pte = 0;
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
		unmap_page((void *)0xEA000000);

		/* Insert the corresponding page directory entry */
		cur_pde = cur_pd->entries[i];
		new_pde = 0;
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
	unmap_page((void *)0xE0000000);

	return new_page_dir_phys;
}
