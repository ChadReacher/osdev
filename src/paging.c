#include "paging.h"
#include "memory.h"
#include "stdio.h"
#include "screen.h"
#include "debug.h"

page_directory_t *cur_page_dir = 0;

void pagefault_handler(registers_state regs) {
	u32 bad_address;

	__asm__ __volatile__ ("movl %%cr2, %0" : "=r"(bad_address));

	kprintf("Page Fault Exception. Error code - %x\n", regs.err_code);
	kprintf("Bad Address: %x\n", bad_address);

	while (1) {
		__asm__ __volatile__ ("hlt");
	}
}

// Return a page for a given virtual address in the current page directory
page_table_entry *get_page(virtual_address addr) {
	// Get current page directory
	page_directory_t *page_dir = cur_page_dir;

	// Get page table in page directory
	page_directory_entry *page_dir_entry = &page_dir->entries[PAGE_DIR_INDEX(addr)];
	page_table_t *table = (page_table_t *)GET_FRAME(*page_dir_entry);
	
	// Get page in table
	page_table_entry *page = &table->entries[PAGE_TABLE_INDEX(addr)];

	return page;
}

void *paging_allocate_page(page_table_entry *page) {
	void *block = allocate_blocks(1);
	if (block) {
		// Set frame address in page to a block's physical address
		*page = ((*page & ~0xFFFFF000) | (physical_address)block);
		// Set present flag in page
		*page |= PAGING_FLAG_PRESENT;
	}
	return block;
}

void paging_free_page(page_table_entry *page) {
	void *phys_addr = (void *)GET_FRAME(*page);
	if (phys_addr) {
		free_blocks(phys_addr, 1);
	}

	*page &= ~PAGING_FLAG_PRESENT;
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

		page_table_entry *page = &table->entries[PAGE_TABLE_INDEX((u32)virt_addr)];
		*page |= PAGING_FLAG_PRESENT;
		*page = ((*page & ~0xFFFFF000) | (physical_address)phys_addr);
		return;
	}

	page_table_t *table = (page_table_t *)(GET_FRAME(*entry));
	page_table_entry *page = &table->entries[PAGE_TABLE_INDEX((u32)virt_addr)];
	*page |= PAGING_FLAG_PRESENT;
	*page = ((*page & ~0xFFFFF000) | (physical_address)phys_addr);
}

void paging_init() {
	register_interrupt_handler(14, pagefault_handler);

	// Create top level page table(page directory)
	page_directory_t *dir = (page_directory_t *)allocate_blocks(1);
	if (!dir)
		return;
	memset((void *)dir, 0, PAGE_DIRECTORY_ENTRIES * sizeof(page_directory_entry));

	// Create default page table, which will correspond to 0-4MB identity mapping
	page_table_t *default_page_table = (page_table_t *)allocate_blocks(1);
	if (!default_page_table)
		return;
	memset((void *)default_page_table, 0, sizeof(page_table_t));

	// Create default page table, which will correspond 
	// to higher half kernel identity mapping
	page_table_t *kernel_page_table = (page_table_t *)allocate_blocks(1);
	if (!kernel_page_table) 
		return;
	memset((void *)kernel_page_table, 0, sizeof(page_table_t));

	// Identity map first 4MB
	for (u32 i = 0, frame = 0x0, virt_addr = 0x0; i < 1024;
			++i, frame += PAGE_SIZE, virt_addr += PAGE_SIZE) {
		page_table_entry page = 0;
		page |= PAGING_FLAG_PRESENT;
		page |= PAGING_FLAG_WRITEABLE;
		page = ((page & ~0xFFFFF000) | (physical_address)frame);

		default_page_table->entries[PAGE_TABLE_INDEX(virt_addr)] = page;
	}
	
	// Identity map higher half kernel to actual physical memory layout
	for (u32 i = 0, frame = 0x10000, virt_addr = 0xC0000000; i < 1024;
			++i, frame += PAGE_SIZE, virt_addr += PAGE_SIZE) {
		page_table_entry page = 0;
		page |= PAGING_FLAG_PRESENT;
		page |= PAGING_FLAG_WRITEABLE;
		page = ((page & ~0xFFFFF000) | (physical_address)frame);

		kernel_page_table->entries[PAGE_TABLE_INDEX(virt_addr)] = page;
	}

	// Set up recursive paging
	page_directory_entry page_dir_entry = 0;
	page_dir_entry |= PAGING_FLAG_PRESENT;
	page_dir_entry |= PAGING_FLAG_WRITEABLE;
	page_dir_entry = ((page_dir_entry & ~0xFFFFF000) | (physical_address)dir);
	dir->entries[PAGE_DIRECTORY_ENTRIES - 1] = page_dir_entry;

	page_directory_entry *default_entry = &dir->entries[PAGE_DIR_INDEX(0x00000000)];
	*default_entry |= PAGING_FLAG_PRESENT;
	*default_entry |= PAGING_FLAG_WRITEABLE;
	*default_entry = ((*default_entry & ~0xFFFFF000) | (physical_address)default_page_table);

	page_directory_entry *kernel_entry = &dir->entries[PAGE_DIR_INDEX(0xC0000000)];
	*kernel_entry |= PAGING_FLAG_PRESENT;
	*kernel_entry |= PAGING_FLAG_WRITEABLE;
	*kernel_entry = ((*kernel_entry & ~0xFFFFF000) | (physical_address)kernel_page_table);

	// Set up top level page directory
	cur_page_dir = dir;

	// Set page directory to CR3 register
	__asm__ __volatile__ ("movl %%eax, %%cr3" : : "a"(cur_page_dir));

	// Tweak 'enable paging' bit in CR0 register
	__asm__ __volatile__ ("movl %cr0, %eax");
	__asm__ __volatile__ ("orl $0x80000001, %eax");
	__asm__ __volatile__ ("movl %eax, %cr0");

	// Identity map framebuffer
	u32 fb_size_in_bytes = SCREEN_SIZE * 4;
	u32 fb_size_in_pages = fb_size_in_bytes / PAGE_SIZE;
	if (fb_size_in_pages % PAGE_SIZE > 0) ++fb_size_in_pages;
	for (u32 i = 0, fb_start = 0xFD000000; i < fb_size_in_pages; ++i, fb_start += PAGE_SIZE) {
		map_page((void *)fb_start, (void *)fb_start);
	}

	DEBUG("%s", "Paging has been initialized\r\n");
}
