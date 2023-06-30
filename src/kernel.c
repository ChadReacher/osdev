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
	kprintf("Number of used or reserved 4K blocks: %x\n", used_blocks);
	kprintf("Number of free 4K blocks: %x\n", (max_blocks - used_blocks));
	kprintf("Total amount of 4K blocks: %x\n", max_blocks);
}

__attribute__ ((section ("kernel_entry"))) void _start() {
	DEBUG("%s has started\r\n", "OS");
	clear_screen();

	int result = init_serial();
	DEBUG("Serial port has been initialized\r\n", "OS");
	if (result == 1) {
		kprintf("Could not initiliaze serial port");
		for (;;) {}
	}

	init_isr();
	DEBUG("ISR has been initialized\r\n", "OS");
	init_timer(50);
	DEBUG("Timer has been initialized\r\n", "OS");
	init_keyboard();
	DEBUG("Keyboard has been initialized\r\n", "OS");

	init_pmm();
	DEBUG("Physical memory manager has been initialized\r\n", "OS");

	kprintf("Physical memory info: ");
	print_physical_memory_info();

	__asm__ ("sti");

	for (;;) {}
}
