#include "serial.h"
#include "port.h"
#include "stdarg.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"
#include "panic.h"

i32 serial_init() {
	port_outb(COM1 + 1, 0x00); // Disable all interrupts
	port_outb(COM1 + 3, 0x80); // Enable DLAB(divisor latch access bit) for baud rate divisor
	port_outb(COM1 + 0, 0x03); // Set divisor to 3 according to 38400 baud(low byte)
	port_outb(COM1 + 0, 0x00); // Set high byte of divisor 3
	port_outb(COM1 + 3, 0x03); // 8N1 Line Protocol: 8 bits, no parity, one stop bit; And clear DLAB
	port_outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14 byte threshold
	port_outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
	port_outb(COM1 + 4, 0x1E); // Set in loopback mode, test the serial chip
	port_outb(COM1 + 0, 0xAB); // Test the serial chip(send byte 0xAB and check if serial returns same byte)

	if (port_inb(COM1 + 0) != 0xAB) {
		PANIC("Could not initiliaze serial port communication");
		return 1;
	}
	
	port_outb(COM1 + 4, 0x0F); // not-loopback with IRQs enabled and OUT1 and OUT2 bits enabled

	DEBUG("%s", "Serial port has been initialized\r\n");
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
	while (*str) {
		write_char_serial(*str++);
	}
}

void serial_printf(i8 *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	i8 internal_buf[1024];
	memset(internal_buf, 0, sizeof internal_buf);

	kvsprintf(internal_buf, fmt, args);
	write_string_serial(internal_buf);
}
