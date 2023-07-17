#include "types.h"
#include "isr.h"
#include "timer.h"
#include "keyboard.h"
#include "screen.h"
#include "stdio.h"
#include "serial.h"
#include "debug.h"
#include "bios_memory_map.h"
#include "pmm.h"
#include "paging.h"
#include "cmos.h"
#include "kshell.h"
#include "syscall.h"
#include "heap.h"

void print_physical_memory_info();

void _start() {
	serial_init();
	DEBUG("%s", "OS has started\r\n");

	isr_init();
	syscall_init();
	irq_init();
	timer_init(50);
	keyboard_init();
	cmos_rtc_init();
	pmm_init();
	paging_init();
	screen_clear();
	kprintf("Physical memory info: ");
	print_physical_memory_info();
	heap_init();

	u8 *p = (u8 *)malloc(5);
	kprintf("Allocated 5 bytes at %p\r\n", p);
	for (u8 i = 1; i < 6; ++i) {
		p[i - 1] = i;
	}
	for (u8 i = 0; i < 5; ++i) {
		kprintf("%d ", p[i]);
	}
	kprintf("\n");

	p = (u8 *)realloc(p, 15);
	kprintf("After REALLOCATING TO 15 bytes at %p\r\n", p);
	for (u8 i = 0; i < 15; ++i) {
		kprintf("%d ", p[i]);
	}
	kprintf("\n");

	for (u8 i = 5; i < 16; ++i) {
		p[i] = i;
	}

	for (u8 i = 0; i < 15; ++i) {
		kprintf("%d ", p[i]);
	}
	kprintf("\n");

	free((void*)p);
	kprintf("%s", "Freed 5 bytes with p\r\n");

	kprintf(PROMPT);
	for (;;) {
		kshell(keyboard_get_last_scancode());
	}
}

void print_physical_memory_info() {
	memory_map_entry *mmap_entry;
	u32 num_entries;

	mmap_entry = (memory_map_entry *)BIOS_MEMORY_MAP;
	num_entries = *((u32 *)BIOS_NUM_ENTRIES);

	kprintf("Total number of entries: %d\n", num_entries);
	for (u8 i = 0; i < num_entries; ++i) {
		kprintf("Region: %x | Base: %x | Length: %x | Type(%d): ", i, (u32)mmap_entry->base_address, (u32)mmap_entry->length, mmap_entry->type);
		switch (mmap_entry->type) {
			case 1:
				kprintf("Available Memory");
				break;
			case 2:
				kprintf("Reserved Memory");
				break;
			case 3:
				kprintf("ACPI Reclaim Memory");
				break;
			case 4:
				kprintf("ACPI NVS Memory");
				break;
			default:
				kprintf("Undefined Memory");
				break;
		}
		kprintf("\n");
		++mmap_entry; 
	}
	kprintf("\n");
	--mmap_entry;
	kprintf("Total amount of memory(in bytes): %x\n", (u32)mmap_entry->base_address + (u32)mmap_entry->length - 1);
}
