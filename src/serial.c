#include "string.h"
#include "port.h"
#include "serial.h"

i32 init_serial() {
	port_outb(COM1 + 1, 0x00); // Disable all interrupts
	port_outb(COM1 + 3, 0x80); // Enable DLAB(divisor latch access bit) for baud rate divisor
	port_outb(COM1 + 0, 0x03); // Set divisor to 3 according to 38400 baud(low byte)
	port_outb(COM1 + 1, 0x00); // Set high byte of divisor 3
	port_outb(COM1 + 3, 0x03); // 8N1 Line Protocol: 8 bits, no parity, one stop bit; And clear DLAB
	port_outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14 byte threshold
	port_outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
	port_outb(COM1 + 4, 0x1E); // Set in loopback mode, test the serial chip
	port_outb(COM1 + 0, 0x80); // Test the serial chip(send byte 0xAB and check if serial returns same byte)

	if (port_inb(COM1 + 0) != 0xAB) {
		return 1;
	}

	
	port_outb(COM1 + 4, 0x0F); // not-loopback with IRQs enabled and OUT1 and OUT2 bits enabled

	return 0;
}

i32 serial_received() {
	return port_inb(COM1 + 5) & 1;
}

i8 read_serial() {
	while (serial_received() == 0);

	return port_inb(COM1);
}

i32 is_transmit_empty() {
	return port_inb(COM1 + 5) & 0x20;
}

void write_char_serial(i8 ch) {
	while (is_transmit_empty() == 0);

	port_outb(COM1, ch);
}

void write_string_serial(i8 *str) {
	size_t sz = strlen(str);
	while (sz--) {
		write_char_serial(*str++);
	}
}
