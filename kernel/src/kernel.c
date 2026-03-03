#include <types.h>
#include <serial.h>
#include <panic.h>
#include <gdt.h>
#include <isr.h>
#include <idt.h>
#include <tss.h>
#include <syscall.h>
#include <timer.h>
#include <keyboard.h>
#include <cmos_rtc.h>
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
#include <bcache.h>
#include <common.h>

void kernel_start(void) {
	serial_init();
	gdt_init();
	isr_init();
	irq_init();
	syscall_init();
	tss_init();
	idt_init();
	timer_init();
	keyboard_init();
	cmos_rtc_init();
	pmm_init();
	paging_init();
	console_init();
	heap_init();
	bcache_init();
	pci_init();
	ata_init();
	scheduler_init();
	// TODO: Implement manual context switch to PID 1
	// - Prepare fake stack, stack frame, irq ret frame, iret frame for PID 1
	// - mount root using disk polling (but when IRQ - remember! no timer
	//   scheduling, maybe it's dangerous to sleep. Actually it should not be,
	//   because idle loop will continously call schedule() and disk IRQ will
	//   fire to move from SLEEP to READY state). But again: we need to
	//   atomically add the request to queue, go to sleep. It can be done by
	//   cli; and during task switch sti(iret);
	// - load /bin/init program into memory
	// - enter user mode (possibly via iret)

	debug("split up to: kmain and kinit\r\n");

	debug("[kmain]: let's manually switch to kinit\r\n");
	schedule();

	debug("[kmain]: entering CPU idle loop\r\n");
	while (1) {
		__asm__ volatile ("hlt");
		schedule();
	}
}

void kinit(void) {
	debug("[kinit]: new kernel process that finishes initialization\r\n");

	mount_root();
	debug("[kinit]: initializing 'init' process...\r\n");
	user_init();
	debug("[kinit]: entering user mode...\r\n");
	user_enter();
}
