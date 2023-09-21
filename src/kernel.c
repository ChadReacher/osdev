#include <types.h>
#include <stdio.h>
#include <serial.h>
#include <panic.h>
#include <debug.h>
#include <gdt.h>
#include <isr.h>
#include <tss.h>
#include <syscall.h>
#include <timer.h>
#include <keyboard.h>
#include <cmos.h>
#include <pmm.h>
#include <paging.h>
#include <screen.h>
#include <heap.h>
#include <process.h>
#include <scheduler.h>
#include <pci.h>
#include <vfs.h>
#include <ata.h>
#include <ext2.h>
#include <elf.h>
#include <string.h>

void _start() {
	serial_init();

	gdt_init();
	isr_init();
	tss_init(5, 0x10, 0);
	irq_init();
	syscall_init();
	timer_init(50);
	keyboard_init();
	cmos_rtc_init();
	pmm_init();
	paging_init();
	screen_init();
	heap_init();
	pci_init();
	vfs_init();
	ata_init();
	ext2_init("/dev/hdb", "/");

	kprintf("/----------------------------------------------\\\n");
	kprintf("                Welcome to the OS.\n");
	kprintf("\\----------------------------------------------/\n");

	vfs_print();

	userinit();
	scheduler_init();

	PANIC("End of kernel\r\n");
}
