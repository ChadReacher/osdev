#include <paging.h>
#include <string.h>
#include <panic.h>
#include <pmm.h>
#include <process.h>
#include <common.h>

static struct page_directory *cur_page_dir = (struct page_directory *)CURR_PAGE_DIR;

static void unmap_page(virtual_address vaddr);

static void flush_tlb(void) {
    __asm__ volatile ("movl %%cr3, %%eax" : : );
    __asm__ volatile ("movl %%eax, %%cr3" : : );
}

void pagefault_handler(struct registers_state *regs) {
    u32 bad_address;

    __asm__ volatile ("movl %%cr2, %0" : "=r"(bad_address));
    const u32 err_code = regs->err_code;

    debug("Page Fault Exception. Bad Address: %#x. Error code: %d\r\n", bad_address, err_code);

    const i8 present = (err_code & PAGING_FLAG_PRESENT) == PAGING_FLAG_PRESENT;
    const i8 rw = (err_code & PAGING_FLAG_WRITEABLE) == PAGING_FLAG_WRITEABLE;
    const i8 us = (err_code & PAGING_FLAG_USER) == PAGING_FLAG_USER;

    if (!present && rw && us) {
        /* Fault due to user heap expansion */
        void *new_heap_page = allocate_blocks(1);
        if (new_heap_page == NULL) {
            panic("Failed to allocate new physical block: not enough space :(");
        }
        map_page((physical_address)new_heap_page, bad_address, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE | PAGING_FLAG_USER);
    } else {
        while (1) {
            __asm__ volatile ("cli");
            __asm__ volatile ("hlt");
        }
    }
}

physical_address virtual_to_physical(virtual_address vaddr) {
    u32 pd_entry_idx = PAGE_DIR_INDEX(vaddr);

    /* Get page table entry from page directory */
    const pd_entry entry = cur_page_dir->entries[pd_entry_idx];
    assert((entry & PAGING_FLAG_PRESENT) == PAGING_FLAG_PRESENT);

    // We use a recursive paging: the last entry in page directory points to the page directory itself
    // It means that it contains the physical address of page directory
    // Suppose the page directory is at 0x117000 physical address. Then
    // PDE[1023] = 0x117000 (discarded the flags for simplicity)
    // If we take this entry, then we will look at the page directory again
    // but now in the "table mode". In "table mode" now each entry corresponds to PHYSICAL frame.
    // *However*, as we look at the page directory that each entry, in fact, is a pointer to a page table, we
    // effectively see their physical address.
    // Then if we take the PTE[1023], we will see the 0x117000 value again.
    // Basically, we reached the physical address of the page directory being in virtual address space.
    // The specific address that corresponds to physical page directory is 0xFFFFF000.
    // Why?
    // Page directory index = 0xFFFFF000 >> 22 = 0x3FF = 1023 i.e. PDE[1023]
    // Page table index = (0xFFFFF000 >> 12) & 0x3FF = 0x3FF = 1023 i.e PTE[1023]
    // Page table offset = 0xFFFFF000 & 0xFFF = 0
    // So, CR3 = 0x117000 (Page Directory)
    // PDE[1023] => 0x117000[1023] = 0x117000 (Page Table) (this IS recursive paging)
    // PTE[1023] => 0x117000[1023] = 0x117000 (Physical Frame) 
    // 0x117000 + 0 (offset) = 0x117000
    // Hooray! We obtained the physical address of the page directory from virtual address
    //
    // In the same manner, we can obtain the addresses of other page tables.
    // We can also get the virtual address of other page tables
    // by working backwards from our _root_ address - 0xFFFFF000
    // If 0xFFFFF000 is the last entry in _page table_, 
    // then the second to last page table is 0xFFFFE000 i.e. PDE[1023] -> PTE[1022]
    // 0xFFFFD000 is PDE[1023] -> PTE[1021]
    // 0xFFFFE000 is PDE[1023] -> PTE[1020]
    const struct page_table *table = (struct page_table *)
        (CURR_PAGE_DIR - (PAGE_DIRECTORY_ENTRIES - 1 - pd_entry_idx) * PAGE_SIZE);

    /* Get page in table */
    const pt_entry page = table->entries[PAGE_TABLE_INDEX(vaddr)];
    assert((page & PAGING_FLAG_PRESENT) == PAGING_FLAG_PRESENT);

    const u32 phys_addr = PAGE_FRAME_BASE(page) + PAGE_FRAME_OFFSET(vaddr);
    return phys_addr;
}

void map_page(physical_address paddr, virtual_address vaddr, u32 flags) {
    pd_entry *entry = &cur_page_dir->entries[PAGE_DIR_INDEX(vaddr)];

    if ((*entry & PAGING_FLAG_PRESENT) != PAGING_FLAG_PRESENT) {
        physical_address new_table_paddr = (physical_address)allocate_blocks(1);
        if (new_table_paddr == 0) {
            panic("Failed to allocate new physical block: not enough space :(");
        }
        *entry |= PAGING_FLAG_PRESENT;
        *entry |= PAGING_FLAG_WRITEABLE;
        *entry = ((*entry & ~0xFFFFF000) | new_table_paddr);

        struct page_table *new_page_table = (struct page_table *)
            (CURR_PAGE_DIR - (PAGE_DIRECTORY_ENTRIES - 1 - PAGE_DIR_INDEX(vaddr)) * PAGE_SIZE);
        memset(new_page_table, 0, sizeof(struct page_table));
    }

    struct page_table *table = (struct page_table *)
        (CURR_PAGE_DIR - (PAGE_DIRECTORY_ENTRIES - 1 - PAGE_DIR_INDEX(vaddr)) * PAGE_SIZE);

    pt_entry *page = &table->entries[PAGE_TABLE_INDEX(vaddr)];
    *page |= flags;
    *page = ((*page & ~0xFFFFF000) | paddr);
}

static void unmap_page(virtual_address vaddr) {
    const pd_entry entry = cur_page_dir->entries[PAGE_DIR_INDEX(vaddr)];
    assert((entry & PAGING_FLAG_PRESENT) == PAGING_FLAG_PRESENT);

    struct page_table *table = (struct page_table *)
        (CURR_PAGE_DIR - (PAGE_DIRECTORY_ENTRIES - 1 - PAGE_DIR_INDEX(vaddr)) * PAGE_SIZE);

    pt_entry *page = &table->entries[PAGE_TABLE_INDEX(vaddr)];
    assert((*page & PAGING_FLAG_PRESENT) == PAGING_FLAG_PRESENT);

    *page &= ~PAGING_FLAG_PRESENT;
    *page = ((*page & ~0xFFFFF000) | 0);

    flush_tlb();
}

void free_user_image(void) {
    u32 kernel_pde_idx = PAGE_DIR_INDEX(KERNEL_LOAD_VADDR);
    for (u32 i = 0; i < kernel_pde_idx; ++i) {
        const pd_entry pde = cur_page_dir->entries[i];
        if ((pde & PAGING_FLAG_PRESENT) != PAGING_FLAG_PRESENT) {
            continue;
        }
        struct page_table *table = (struct page_table *)
            (CURR_PAGE_DIR - (PAGE_DIRECTORY_ENTRIES - 1 - i) * PAGE_SIZE);
        for (u32 j = 0; j < PAGE_TABLE_ENTRIES; ++j) {
            const pt_entry pte = table->entries[j];
            if ((pte & PAGING_FLAG_PRESENT) != PAGING_FLAG_PRESENT) {
                continue;
            }
            void *page_frame = (void *)PAGE_FRAME_BASE(pte);
            free_blocks(page_frame, 1);
            table->entries[j] = 0;
        }
        physical_address table_frame = PAGE_FRAME_BASE(pde);
        free_blocks((void*)table_frame, 1);
        cur_page_dir->entries[i] = 0;
    }

    flush_tlb();
}

struct page_directory *paging_copy_page_dir(bool is_deep_copy) {
    u32 kernel_pde_idx = PAGE_DIR_INDEX(KERNEL_LOAD_VADDR);

    void *new_pd_phys = (struct page_directory *)allocate_blocks(1);
    if (new_pd_phys == NULL) {
        debug("Failed to allocate new physical block: not enough space :(");
        return NULL;
    }
    map_page((physical_address)new_pd_phys, 0xE0000000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
    struct page_directory *new_pd = (struct page_directory *)0xE0000000;
    memset(new_pd, 0, PAGE_DIRECTORY_ENTRIES * 4);

    // Copy the kernel pages (starting from 0xC0000000)
    for (u32 i = kernel_pde_idx; i < PAGE_DIRECTORY_ENTRIES; ++i) {
        new_pd->entries[i] = cur_page_dir->entries[i];
    }
    // Setup the recursive paging
    new_pd->entries[PAGE_DIRECTORY_ENTRIES - 1] = (u32)new_pd_phys | PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE;

    if (!is_deep_copy) {
        unmap_page(0xE0000000);
        return new_pd_phys;
    }

    /* Deep copy user pages */
    for (u32 i = 0; i < kernel_pde_idx; ++i) {
        const pd_entry curr_pde = cur_page_dir->entries[i];
        if ((curr_pde & PAGING_FLAG_PRESENT) != PAGING_FLAG_PRESENT) {
            continue;
        }

        const struct page_table *table = (struct page_table *)
            (CURR_PAGE_DIR - (PAGE_DIRECTORY_ENTRIES - 1 - i) * PAGE_SIZE);

        struct page_table *new_table_phys = allocate_blocks(1);
        if (new_table_phys == NULL) {
            debug("Failed to allocate new physical block: not enough space :(");
            return NULL;
        }
        map_page((physical_address)new_table_phys, 0xEA000000, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
        struct page_table *new_table = (struct page_table *)0xEA000000;
        memset(new_table, 0, PAGE_TABLE_ENTRIES * 4);

        for (u32 j = 0; j < PAGE_TABLE_ENTRIES; ++j) {
            const pt_entry curr_pte = table->entries[j];
            if ((curr_pte & PAGING_FLAG_PRESENT) != PAGING_FLAG_PRESENT) {
                continue;
            }

            /* Copy the page frame's contents  */
            const void *curr_page_frame = (void *)PAGE_FRAME_BASE(curr_pte);
            void *new_page_frame = allocate_blocks(1);
            if (new_page_frame == NULL) {
                debug("Failed to allocate new physical block: not enough space :(");
                return NULL;
            }
            map_page((physical_address)curr_page_frame, 0xEB000000,
                    PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
            map_page((physical_address)new_page_frame, 0xEC000000,
                    PAGING_FLAG_PRESENT | PAGING_FLAG_WRITEABLE);
            memcpy((void *)0xEC000000, (void *)0xEB000000, PAGE_TABLE_ENTRIES * 4);
            unmap_page(0xEC000000);
            unmap_page(0xEB000000);

            /* Insert the corresponding page table entry */
            const pt_entry new_pte = ((curr_pte & ~0xFFFFF000) | (u32)new_page_frame);
            new_table->entries[j] = new_pte;
        }
        unmap_page(0xEA000000);

        /* Insert the corresponding page directory entry */
        const pd_entry new_pde = ((curr_pde & ~0xFFFFF000) | (u32)new_table_phys);
        new_pd->entries[i] = new_pde;
    }
    unmap_page(0xE0000000);

    return new_pd_phys;
}

void paging_init(void) {
    register_interrupt_handler(14, pagefault_handler);

    debug("Paging has been initialized\r\n");
}
