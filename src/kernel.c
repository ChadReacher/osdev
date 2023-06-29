#include "types.h"
#include "isr.h"
#include "timer.h"
#include "keyboard.h"
#include "screen.h"
#include "stdio.h"
#include "serial.h"
#include "debug.h"
#include "bios_memory_map.h"

void print_physical_memory_info() {
	u32 num_entries;

	num_entries = *((u32 *)0x8500);
	kprintf("Total number of entries: %d\n", num_entries);
	for (u8 i = 0; i < num_entries; ++i) {
		kprintf("Region: %x | Base: %x | Length: %x | Type: ", i, (u32)bios_memory_map->base_address, (u32)bios_memory_map->length);
		switch (bios_memory_map->type) {
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
		++bios_memory_map; 
	}
	kprintf("\n");
	--bios_memory_map;
	kprintf("Total amount of memory(in bytes): %x\n", (u32)bios_memory_map->base_address + (u32)bios_memory_map->length - 1);
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

	kprintf("Physical memory info: ");
	print_physical_memory_info();

	__asm__ ("sti");

	for (;;) {}
}
