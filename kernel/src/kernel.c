#include <types.h>
#include <stdio.h>
#include <serial.h>
#include <panic.h>
#include <gdt.h>
#include <isr.h>
#include <idt.h>
#include <tss.h>
#include <syscall.h>
#include <timer.h>
#include <keyboard.h>
#include <cmos.h>
#include <pmm.h>
#include <paging.h>
#include <console.h>
#include <screen.h>
#include <heap.h>
#include <process.h>
#include <scheduler.h>
#include <pci.h>
#include <ata.h>
#include <ext2.h>
#include <elf.h>

void _start() {
	serial_init();
	gdt_init();
	isr_init();
	irq_init();
	syscall_init();
	tss_init(5, 0x10, 0);
	idt_init();
	/*__asm__ __volatile__ ("sti");*/
	timer_init(TIMER_FREQ);
	keyboard_init();
	cmos_rtc_init();
	pmm_init();
	paging_init();
	console_init();

	/*screen_init();*/
	heap_init();
	pci_init();
	ata_init();
	scheduler_init();
	mount_root();
	user_init();

	/*i32 fd = syscall_open("/dev/tty0");
	syscall_dup(fd);
	syscall_dup(fd);
	i8 m[] = "Hello world from TTY!";
	syscall_write(1, m, sizeof(m));*/
	enter_usermode();

	panic("End of kernel\r\n");
	for (;;);
}
