#ifndef PAGING_H
#define PAGING_H

#include "types.h"
#include "pmm.h"
#include "isr.h"

#define PAGE_TABLE_ENTRIES 1024
#define PAGE_DIRECTORY_ENTRIES 1024
#define PAGE_SIZE 4096

#define PAGE_DIR_INDEX(addr) ((addr) >> 22)
#define PAGE_TABLE_INDEX(addr) (((addr) >> 12) & 0x3FF)
#define PAGE_FRAME_INDEX(addr) ((addr) & 0xFFF)
#define GET_FRAME(entry) ((entry) & 0xFFFFF000)

#define PAGING_FLAG_PRESENT 0x01
#define PAGING_FLAG_WRITEABLE 0x02
#define PAGING_FLAG_USER 0x04
#define PAGING_FLAG_ACCESSED 0x20
#define PAGING_FLAG_DIRTY 0x40

typedef u32 virtual_address;
typedef u32 physical_address;

typedef u32 page_table_entry;
typedef u32 page_directory_entry;

typedef struct {
	page_table_entry entries[PAGE_TABLE_ENTRIES];
} page_table_t;

typedef struct {
	page_directory_entry entries[PAGE_DIRECTORY_ENTRIES];
} page_directory_t;

page_table_entry *paging_get_page(virtual_address addr);
void *paging_allocate_page(page_table_entry *page);
void paging_free_page(page_table_entry *page);
void map_page(void *phys_addr, void *virt_addr, u32 flags);
void unmap_page(void *virt_addr);
page_table_entry *get_page(virtual_address addr);
void paging_init();
void pagefault_handler(registers_state *regs);
void *virtual_to_physical(void *virt_addr);
page_directory_t *paging_copy_page_dir(bool is_deep_copy);

#endif
