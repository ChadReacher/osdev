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
#include <bcache.h>
#include <rtl8139.h>

void kernel_start(void) {
	serial_init();
	gdt_init();
	isr_init();
	irq_init();
	syscall_init();
	tss_init(5, 0x10, 0);
	idt_init();
	timer_init(TIMER_FREQ);
	keyboard_init();
	cmos_rtc_init();
	pmm_init();
	paging_init();
	console_init();
	heap_init();
	bcache_init();
	pci_init();
	ata_init();
	__asm__ ("sti");
	rtl8139_init();

	/* Test sending Ethernet frame */
	/*u8 dest_mac[6] = { 0x06, 0xff, 0xbf, 0x93, 0xe8, 0x88 };
#define X 1500
	u8 *frame = malloc(X);
	int i;
	for (i = 0; i < X; ++i) {
		frame[i] = ('0' + (i % 9));
	}
	frame[1498] = 'X';
	frame[1499] = 'Y';
	ethernet_send_frame(dest_mac, 0x0800, frame, X);
	free(frame);*/

	u8 dst_ip[4] = { 192, 168, 0, 115 };
	arp_send_packet(dst_ip);

	/*scheduler_init();
	mount_root();
	user_init();
	enter_usermode();
	panic("End of kernel\r\n");*/

	debug("END OF KERNEL............\r\n");
	for (;;);
}
