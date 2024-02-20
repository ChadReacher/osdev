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
	timer_init(TIMER_FREQ);
	keyboard_init();
	cmos_rtc_init();
	pmm_init();
	paging_init();
	screen_init();
	heap_init();
	pci_init();
	//vfs_init();
	ata_init();

	extern ata_device_t devices[4];
	u32 sector = 0, nsect = 1;
	i8 *buf = ata_read(&devices[0], sector, nsect);
	for (u32 i = 0; i < 512; i += 16) {
		kprintf("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x "
				"0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
				buf[i + 0], buf[i + 1], buf[i + 2], buf[i + 3],
				buf[i + 4], buf[i + 5], buf[i + 6], buf[i + 7],
				buf[i + 8], buf[i + 9], buf[i + 10], buf[i + 11],
				buf[i + 12], buf[i + 13], buf[i + 14], buf[i + 15]);
	}
	buf = malloc(1024);
	kprintf("got buf: %p\n", buf);
	memset(buf+512*0, 0x12, 512);
	memset(buf+512*1, 0x34, 512);

	sector = 2, nsect = 2;
	ata_write(&devices[0], sector, nsect, buf);

	for (;;);

	//ext2_init("/dev/hdb", "/");
	//kprintf("/----------------------------------------------\\\n");
	//kprintf("                Welcome to the OS.\n");
	//kprintf("\\----------------------------------------------/\n");
	//vfs_print();
	//scheduler_init();
	//userinit();

	//PANIC("End of kernel\r\n");
}
