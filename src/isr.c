#include "idt.h"
#include "stdio.h"
#include "port.h"
#include "pic.h"
#include "isr.h"
#include "debug.h"

isr_t interrupt_handlers[256];

i8 *exception_messages[] = {
	"Division by zero",
	"Debug",
	"Non Maskable Interrupt",
	"Breakpoint",
	"into Detected Overflow",
	"Out of Bounds",
	"Invalid Opcode",
	"No Coprocessor",

	"Double Fault",
	"Coprocessor Segment Overrun",
	"Bad TSS",
	"Segment Not Present",
	"Stack Fault",
	"General Protection Fault",
	"Page Fault",
	"Unknown Interrupt",
	
	"Coprocessor Fault",
	"Alignment Check",
	"Machine Check",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",

	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
};

void isr_init() {
	pic_remap();
	DEBUG("%s", "PIC has been remapped");

	idt_set(0, (u32)isr0, 0x8E);
	idt_set(1, (u32)isr1, 0x8E);
	idt_set(2, (u32)isr2, 0x8E);
	idt_set(3, (u32)isr3, 0x8E);
	idt_set(4, (u32)isr4, 0x8E);
	idt_set(5, (u32)isr5, 0x8E);
	idt_set(6, (u32)isr6, 0x8E);
	idt_set(7, (u32)isr7, 0x8E);
	idt_set(8, (u32)isr8, 0x8E);
	idt_set(9, (u32)isr9, 0x8E);
	idt_set(10, (u32)isr10, 0x8E);
	idt_set(11, (u32)isr11, 0x8E);
	idt_set(12, (u32)isr12, 0x8E);
	idt_set(13, (u32)isr13, 0x8E);
	idt_set(14, (u32)isr14, 0x8E);
	idt_set(15, (u32)isr15, 0x8E);
	idt_set(16, (u32)isr16, 0x8E);
	idt_set(17, (u32)isr17, 0x8E);
	idt_set(18, (u32)isr18, 0x8E);
	idt_set(19, (u32)isr19, 0x8E);
	idt_set(20, (u32)isr20, 0x8E);
	idt_set(21, (u32)isr21, 0x8E);
	idt_set(22, (u32)isr22, 0x8E);
	idt_set(23, (u32)isr23, 0x8E);
	idt_set(24, (u32)isr24, 0x8E);
	idt_set(25, (u32)isr25, 0x8E);
	idt_set(26, (u32)isr26, 0x8E);
	idt_set(27, (u32)isr27, 0x8E);
	idt_set(28, (u32)isr28, 0x8E);
	idt_set(29, (u32)isr29, 0x8E);
	idt_set(30, (u32)isr30, 0x8E);
	idt_set(31, (u32)isr31, 0x8E);

	idt_set(32, (u32)irq0, 0x8E);
	idt_set(33, (u32)irq1, 0x8E);
	idt_set(34, (u32)irq2, 0x8E);
	idt_set(35, (u32)irq3, 0x8E);
	idt_set(36, (u32)irq4, 0x8E);
	idt_set(37, (u32)irq5, 0x8E);
	idt_set(38, (u32)irq6, 0x8E);
	idt_set(39, (u32)irq7, 0x8E);
	idt_set(40, (u32)irq8, 0x8E);
	idt_set(41, (u32)irq9, 0x8E);
	idt_set(42, (u32)irq10, 0x8E);
	idt_set(43, (u32)irq11, 0x8E);
	idt_set(44, (u32)irq12, 0x8E);
	idt_set(45, (u32)irq13, 0x8E);
	idt_set(46, (u32)irq14, 0x8E);
	idt_set(47, (u32)irq15, 0x8E);

	init_idt();
	DEBUG("%s", "IDT has been initialized\r\n");
	DEBUG("%s", "ISRs have been initialized\r\n");
}

void register_interrupt_handler(u8 n, isr_t handler) {
	interrupt_handlers[n] = handler;
}

void isr_handler(registers_t r) {
	if (interrupt_handlers[r.int_number] != 0) {
		isr_t handler = interrupt_handlers[r.int_number];
		handler(r);
	} else {
		kprintf("Received interrupt: %s(%d) with error code: %x\n", exception_messages[r.int_number], r.int_number, r.err_code);
		__asm__("hlt");
	}
}

void irq_handler(registers_t r) {
	// Sending EOI command code
	if (r.int_number >= 40) {
		port_outb(0xA0, 0x20); // slave
	}
	port_outb(0x20, 0x20); // master

	if (interrupt_handlers[r.int_number] != 0) {
		isr_t handler = interrupt_handlers[r.int_number];
		handler(r);
	}
}
