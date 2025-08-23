#ifndef PAGING_H
#define PAGING_H

#include "types.h"

#define CURR_PAGE_DIR (0xFFFFF000)

#define PAGE_DIRECTORY_ENTRIES 1024
#define PAGE_TABLE_ENTRIES 1024
#define PAGE_SIZE 4096

// Virtual address consists of three parts:
// [ C ] [ B ] [ A ]
// A: 0-11th bits  - page offset
// B: 12-21st bits - index in a page table
// C: 22-31st bits - index in a page directory
typedef u32 virtual_address;
typedef u32 physical_address;

#define PAGE_DIR_INDEX(addr) ((addr) >> 22)
#define PAGE_TABLE_INDEX(addr) (((addr) >> 12) & 0x3FF)
#define PAGE_FRAME_OFFSET(addr) ((addr) & 0xFFF)
#define PAGE_FRAME_BASE(entry) ((entry) & 0xFFFFF000)

#define PAGING_FLAG_PRESENT     (1 << 0)
#define PAGING_FLAG_WRITEABLE   (1 << 1)
#define PAGING_FLAG_USER        (1 << 2)
#define PAGING_FLAG_ACCESSED    (1 << 5)
#define PAGING_PTE_FLAG_DIRTY   (1 << 6)

typedef u32 pd_entry;

// Page table entry
// Present: 1 bit
// Read/Write: 1 bit
// User: 1 bit
// Dirty: 1 bit
// ...
// Physical page address: 20 bits
typedef u32 pt_entry;

struct page_table {
	pt_entry entries[PAGE_TABLE_ENTRIES];
};

struct page_directory {
    pd_entry entries[PAGE_DIRECTORY_ENTRIES];
};

void paging_init(void);
physical_address virtual_to_physical(virtual_address vaddr);

void map_page(physical_address paddr, virtual_address vaddr, u32 flags);

void free_user_image(void);
struct page_directory *paging_copy_page_dir(bool is_deep_copy);

#endif
