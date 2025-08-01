#include <serial.h>
#include <port.h>
#include <panic.h>

static i32 serial_received(void) {
	return port_inb(COM1 + 5) & 1;
}

static i32 is_transmit_empty(void) {
	return port_inb(COM1 + 5) & 0x20;
}

void serial_init(void) {
	port_outb(COM1 + 1, 0x00); /* Disable all interrupts */
	port_outb(COM1 + 3, 0x80); /* Enable DLAB(divisor latch access bit) for baud rate divisor */
	port_outb(COM1 + 0, 0x03); /* Set divisor to 3 according to 38400 baud(low byte) */
	port_outb(COM1 + 0, 0x00); /* Set high byte of divisor 3 */
	port_outb(COM1 + 3, 0x03); /* 8N1 Line Protocol: 8 bits, no parity, one stop bit; And clear DLAB */
	port_outb(COM1 + 2, 0xC7); /* Enable FIFO, clear them, with 14 byte threshold */
	port_outb(COM1 + 4, 0x0B); /* IRQs enabled, RTS/DSR set */
	port_outb(COM1 + 4, 0x1E); /* Set in loopback mode, test the serial chip */
	port_outb(COM1 + 0, 0xAB); /* Test the serial chip(send byte 0xAB and check if serial returns the same byte) */

	if (port_inb(COM1 + 0) != 0xAB) {
		__asm__ volatile ("cli; hlt");
		for (;;);
	}
	
	port_outb(COM1 + 4, 0x0F); /* not-loopback with IRQs enabled and OUT1 and OUT2 bits enabled */

	debug("Serial port has been initialized\r\n");
}


i8 read_serial(void) {
	while (serial_received() == 0);

	return port_inb(COM1);
}

void write_serial(const i8 *str) {
	while (*str) {
		u8 ch = *str++;
		while (is_transmit_empty() == 0);
		port_outb(COM1, ch);
	}
}

