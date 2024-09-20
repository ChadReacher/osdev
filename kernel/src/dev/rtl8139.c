#include <rtl8139.h>
#include <pci.h>
#include <panic.h>
#include <port.h>
#include <heap.h>
#include <paging.h>
#include <string.h>
#include <ethernet.h>

union pci_device rtl8139_device;
static u32 ioaddr = 0;
static u8 TSAD[4] = { 0x20, 0x24, 0x28, 0x2C };
static u8 TSD[4]  = { 0x10, 0x14, 0x18, 0x1C };
static u8 curr_tx_pair = 0;

static u8 *tx_buffer = NULL;
static u32 tx_buffer_phys = 0;
static u8 volatile *rx_buffer = NULL;
static u32 rx_index = 0;
u8 my_mac[6] = {0};


void rtl8139_transmit_data(void *data, u32 len) {
    memcpy(tx_buffer, data, len);
    port_outl(ioaddr + TSAD[curr_tx_pair], tx_buffer_phys);
    port_outl(ioaddr + TSD[curr_tx_pair], len);
    curr_tx_pair = (curr_tx_pair + 1) % 4;
}

/* We receive:
 * rx status as frame header - 2 bytes
 * rx size (including size of CRC) - 2 bytes
 * and the rest is data including CRC value.
 */
void rtl8139_receive_data() {
    u8 *buf = (u8 *)rx_buffer;
    u32 idx = rx_index;

    /* Wait for buffer to become empty */
    while ((port_inb(ioaddr + CMD_REG) & RxBufEmpty) == 0) {
        u32 offset = idx % RX_BUFFER_SZ;
        u16 rx_status = ((u16 *)(buf + offset))[0];
        u16 rx_len = ((u16 *)(buf + offset))[1]; /* Including CRC */
        if ((rx_status & 0x1) == 1) {
            u16 frame_len = rx_len - 4; /* Omit CRC */
            
            u8 *data = malloc(frame_len * sizeof(u8));
            memcpy(data, &buf[4 + offset], frame_len);

            ethernet_receive_frame(data, frame_len);
            free(data);
        } else {
            debug("Bad packet...\r\n");
	}

        /* 4 bytes for header length, 3 bytes for dword alignment */
        idx = (idx + rx_len + 4 + 3) & RX_READ_POINTER_MASK;

        /* - 16 to avoid overflow */
        port_outw(ioaddr + RXBUFPTR_REG, idx - 16);
    }
    rx_index = idx;
}

void rtl8139_handler(struct registers_state *regs) {
	(void)regs;
	u16 status = port_inw(ioaddr + ISR_REG);
	/* Reset the Rx and Tx bits */
	port_outw(ioaddr + ISR_REG, ROK | TOK);
	if (status & TOK) {
		debug("A frame has been successfully transmitted\r\n");
	} else if (status & ROK) {
		debug("A frame has been successfully received\r\n");
		rtl8139_receive_data();
	}
}

void rtl8139_init() {
	u32 rx_buffer_phys, irq_num, mac_part1, mac_part2;
	u16 pci_command;
	rtl8139_device = pci_get_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, -1);

	/* Enable PCI Bus Mastering */
	pci_command = pci_read(rtl8139_device, PCI_COMMAND);
	if (!(pci_command & (1 << 2))) {
		pci_command |= (1 << 2);
		pci_write(rtl8139_device, PCI_COMMAND, pci_command);
	}

	/* Get ioaddr */
	ioaddr = pci_read(rtl8139_device, PCI_BAR0);
	ioaddr &= (~(0x3));

	/* Turning on the RTL8139 */
	port_outb(ioaddr + CONF1_REG, 0x0);

	/* Software Reset */
	port_outb(ioaddr + CMD_REG, CmdReset);
	/* Wait for reset to end */
	while ((port_inb(ioaddr + CMD_REG) & CmdReset) != 0);
    
	/* Init transmit buffer */
	tx_buffer = (u8 *)malloc(TX_BUFFER_SZ);
	memset(tx_buffer, 0, TX_BUFFER_SZ);
	tx_buffer_phys = (u32)virtual_to_physical((void *)tx_buffer);

	/* Init receive buffer, set up additional spaces
	 * when frame overflows the buffer (8K) */
	rx_buffer = malloc(RX_BUFFER_SZ + 16 + 1500);
	rx_buffer_phys = (u32)virtual_to_physical((void *)rx_buffer);
	port_outl(ioaddr + RXBUF_REG, rx_buffer_phys);

	/* Set Interrupt Mask Register and ISR */
	port_outw(ioaddr + IMR_REG, RxOk | TxOk);

	/* Configure receive buffer(RCR) */
	port_outl(ioaddr + RX_CONF_REG, AcceptAllPhys | AcceptMyPhys
		       	| AcceptMulticast | AcceptBroadcast | DoWrap);

	/* Enable Receive and Trasmitter */
	port_outb(ioaddr + CMD_REG, RxEnable | TxEnable);

	/* Enable IRQ handler */
	irq_num = pci_read(rtl8139_device, PCI_INTERRUPT_LINE);
	clear_irq_mask(irq_num);
	register_interrupt_handler(32 + irq_num, rtl8139_handler);

	/* Read MAC address */
	mac_part1 = port_inl(ioaddr + ID_REG0);
	mac_part2 = port_inw(ioaddr + ID_REG4);

	my_mac[0] = mac_part1 >> 0;
	my_mac[1] = mac_part1 >> 8;
	my_mac[2] = mac_part1 >> 16;
	my_mac[3] = mac_part1 >> 24;
	my_mac[4] = mac_part2 >> 0;
	my_mac[5] = mac_part2 >> 8;
	debug("Got MAC address - %x:%x:%x:%x:%x:%x\r\n",
		my_mac[0], my_mac[1], my_mac[2], my_mac[3], my_mac[4], my_mac[5]
	);
}
