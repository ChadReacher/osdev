#include "k_stdio.h"
#include "idt.h"
#include "isr.h"

void init_isrs() {
	idt_set(0, (u32*)isr0, INTERRUPT_GATE_TYPE);
	idt_set(1, (u32*)isr1, INTERRUPT_GATE_TYPE);
	idt_set(2, (u32*)isr2, INTERRUPT_GATE_TYPE);
	idt_set(3, (u32*)isr3, INTERRUPT_GATE_TYPE);
	idt_set(4, (u32*)isr4, INTERRUPT_GATE_TYPE);
	idt_set(5, (u32*)isr5, INTERRUPT_GATE_TYPE);
	idt_set(6, (u32*)isr6, INTERRUPT_GATE_TYPE);
	idt_set(7, (u32*)isr7, INTERRUPT_GATE_TYPE);
	idt_set(8, (u32*)isr8, INTERRUPT_GATE_TYPE);
	idt_set(9, (u32*)isr9, INTERRUPT_GATE_TYPE);
	idt_set(10, (u32*)isr10, INTERRUPT_GATE_TYPE);
	idt_set(11, (u32*)isr11, INTERRUPT_GATE_TYPE);
	idt_set(12, (u32*)isr12, INTERRUPT_GATE_TYPE);
	idt_set(13, (u32*)isr13, INTERRUPT_GATE_TYPE);
	idt_set(14, (u32*)isr14, INTERRUPT_GATE_TYPE);
	idt_set(15, (u32*)isr15, INTERRUPT_GATE_TYPE);
	idt_set(16, (u32*)isr16, INTERRUPT_GATE_TYPE);
	idt_set(17, (u32*)isr17, INTERRUPT_GATE_TYPE);
	idt_set(18, (u32*)isr18, INTERRUPT_GATE_TYPE);
	idt_set(19, (u32*)isr19, INTERRUPT_GATE_TYPE);
	idt_set(20, (u32*)isr20, INTERRUPT_GATE_TYPE);
	idt_set(21, (u32*)isr21, INTERRUPT_GATE_TYPE);
	idt_set(22, (u32*)isr22, INTERRUPT_GATE_TYPE);
	idt_set(23, (u32*)isr23, INTERRUPT_GATE_TYPE);
	idt_set(24, (u32*)isr24, INTERRUPT_GATE_TYPE);
	idt_set(25, (u32*)isr25, INTERRUPT_GATE_TYPE);
	idt_set(26, (u32*)isr26, INTERRUPT_GATE_TYPE);
	idt_set(27, (u32*)isr27, INTERRUPT_GATE_TYPE);
	idt_set(28, (u32*)isr28, INTERRUPT_GATE_TYPE);
	idt_set(29, (u32*)isr29, INTERRUPT_GATE_TYPE);
	idt_set(30, (u32*)isr30, INTERRUPT_GATE_TYPE);
	idt_set(31, (u32*)isr31, INTERRUPT_GATE_TYPE);

	init_idt();
}

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

void isr_handler(registers_t r) {
	kprintf("Received interrupt: %s(%d) with error code: %x\n", exception_messages[r.int_number], r.int_number, r.err_code);
}
