#include <rtl8139.h>
#include <pci.h>
#include <debug.h>
#include <port.h>
#include <heap.h>
#include <paging.h>

pci_device_t rtl8139_device;
static u32 ioaddr = 0;
static u8 *rx_buffer = NULL;
static u32 rx_index = 0;
static u8 mac_address[6] = {0};

void rtl8139_handler(registers_state *regs) {
	(void)regs;
	u16 status = port_inw(ioaddr + 0x3E);
	port_outw(ioaddr + 0x3E, 0x5);
	//port_outw(ioaddr + 0x3E, status);
	if (status & (1 << 2)) {
		DEBUG("%s", "A frame has been transmitted\r\n");
	}

	if (status & (1 << 0)) {
		DEBUG("%s", "A frame has been received \r\n");
		rtl8139_receive_frame();
	}
}

void rtl8139_receive_frame() {
}

void rtl8139_init() {
	rtl8139_device = pci_get_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, -1);

	// Enable PCI Bus Mastering
	u16 pci_command = pci_read(rtl8139_device, PCI_COMMAND);
	if (!(pci_command & (1 << 2))) {
		pci_command |= (1 << 2);
		pci_write(rtl8139_device, PCI_COMMAND, pci_command);
	}

	// Get ioaddr
	ioaddr = pci_read(rtl8139_device, PCI_BAR0);
	ioaddr &= (~(0x3));

	// Turning on the RTL8139
	port_outb(ioaddr + 0x52, 0x0);

	// Software Reset
	port_outb(ioaddr + 0x37, 0x10);
	while ((port_inb(ioaddr + 0x37) & 0x10) != 0);

	// Init receive buffer
	rx_buffer = malloc(RX_BUFFER_SZ);
	u32 rx_buffer_phys = (u32)virtual_to_physical((void *)rx_buffer);
	port_outl(ioaddr + 0x30, rx_buffer_phys);

	// Set IMR and ISR
	port_outl(ioaddr + 0x3C, 0x0005);

	// Configure receive buffer(RCR)
	port_outl(ioaddr + 0x44, 0xF | (1 << 7));

	// Enable Receive and Trasmitter	
	port_outb(ioaddr + 0x37, 0xC);

	// Enable IRQ handler
	u32 irq_num = pci_read(rtl8139_device, PCI_INTERRUPT_LINE);
	register_interrupt_handler(irq_num, rtl8139_handler);

	// Read MAC address
	u32 mac_part1 = port_inl(ioaddr + 0x0);
	u32 mac_part2 = port_inw(ioaddr + 0x4);

	mac_address[0] = mac_part1 >> 0;
	mac_address[1] = mac_part1 >> 8;
	mac_address[2] = mac_part1 >> 16;
	mac_address[3] = mac_part1 >> 24;
	mac_address[4] = mac_part2 >> 0;
	mac_address[5] = mac_part2 >> 8;

	DEBUG("MAC address - %x:%x:%x:%x:%x:%x\r\n",
			mac_address[0],
            mac_address[1],
            mac_address[2],
            mac_address[3],
            mac_address[4],
            mac_address[5]
			);
}



