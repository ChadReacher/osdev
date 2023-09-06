#include <pic.h>
#include <port.h>

void pic_disable() {
	port_outb(PIC2_DATA, 0xFF);
	port_outb(PIC1_DATA, 0xFF);
}

// Set IRQ mask by setting the bit in the IMR (interrupt mask register)
// This will ignore the IRQ
void set_irq_mask(u8 irq_line) {
	u16 port;
	u8 value;

	if (irq_line < 8) {
		port = PIC1_DATA;
	} else {
		port = PIC2_DATA;
		irq_line -= 8;
	}
	
	// Get current IMR value, set on the IRQ bit to mask it
	value = port_inb(port) | (1 << irq_line);
	
	// Write new value to IMR
	port_outb(port, value);
}

// Clear IRQ mask by clearing the bit in the IMR (interrupt mask register)
// This will enable the IRQ
void clear_irq_mask(u8 irq_line) {
	u16 port;
	u8 value;

	if (irq_line < 8) {
		port = PIC1_DATA;
	} else {
		port = PIC2_DATA;
		irq_line -= 8;
	}
	
	// Get current IMR value, set on the IRQ bit to unmask it
	value = port_inb(port) | ~(1 << irq_line);
	
	// Write new value to IMR
	port_outb(port, value);
}

void pic_remap() {
	u8 pic1_mask, pic2_mask;

	// Save current masks
	pic1_mask = port_inb(PIC1_DATA);
	pic2_mask = port_inb(PIC2_DATA);

	// ICW 1 (Initialiazation control word) - bit 0 = send up to ICW 4, bit 4 = initialize PIC
	port_outb(PIC1_CMD, 0x11);
	port_outb(0x80, 0x0); // io wait
	port_outb(PIC2_CMD, 0x11);
	port_outb(0x80, 0x0); // io wait

	// ICW 2 - Where to map the base interrupt in the IDT
	port_outb(PIC1_DATA, NEW_IRQ_START_PRIMARY);
	port_outb(0x80, 0x0); // io wait
	port_outb(PIC2_DATA, NEW_IRQ_START_SLAVE);
	port_outb(0x80, 0x0); // io wait

	// ICW 3 - Where to map PIC2 to the IRQ line in PIC1
	port_outb(PIC1_DATA, 0x4);
	port_outb(0x80, 0x0); // io wait
	port_outb(PIC2_DATA, 0x2);
	port_outb(0x80, 0x0); // io wait

	// ICW 4 - Set x86 mode
	port_outb(PIC1_DATA, 0x1);
	port_outb(0x80, 0x0); // io wait
	port_outb(PIC2_DATA, 0x1);
	port_outb(0x80, 0x0); // io wait

	// Save current masks
	port_outb(PIC1_DATA, pic1_mask);
	port_outb(PIC2_DATA, pic2_mask);

}

void pic_send_eoi(u8 irq) {
	if (irq >= 8) {
		port_outb(PIC2_CMD, PIC_EOI);
	}
	port_outb(PIC1_CMD, PIC_EOI);
}
