#ifndef PCI_H
#define PCI_H

#include "types.h"

// Ports
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// Offset
#define PCI_VENDOR_ID 0x00
#define PCI_DEVICE_ID 0x02
#define PCI_COMMAND 0x04
#define PCI_STATUS 0x06
#define PCI_REVISION_ID 0x08
#define PCI_PROG_IF 0x09
#define PCI_SUBCLASS 0x0A
#define PCI_CLASS 0x0B
#define PCI_CACHE_LINE_SIZE 0x0C
#define PCI_LATENCY_TIMER 0x0D
#define PCI_HEADER_TYPE 0x0E
#define PCI_BIST 0x0F
#define PCI_BAR0 0x10
#define PCI_BAR1 0x14
#define PCI_BAR2 0x18
#define PCI_BAR3 0x1C
#define PCI_BAR4 0x20
#define PCI_BAR5 0x24
#define PCI_INTERRUPT_LINE 0x3C
#define PCI_SECONDARY_BUS 0x09

// Device type
#define PCI_HEADER_TYPE_DEVICE 0
#define PCI_HEADER_TYPE_BRIDGE 1
#define PCI_HEADER_TYPE_CARDBUS 2
#define PCI_TYPE_BRIDGE 0x0604
#define PCI_TYPE_SATA 0x0106
#define PCI_NONE 0xFFFF


#define DEVICE_PER_BUS 32
#define FUNCTION_PER_DEVICE 8

typedef union pci_device {
	u32 bits;
	struct {
		u32 always_zero		: 2;
		u32 field_num		: 6;
		u32 function_num	: 3;
		u32 device_num		: 5;
		u32 bus_num			: 8;
		u32 reserved		: 7;
		u32	enable_bit		: 1;
	};
} pci_device_t;

void pci_init();
u32 pci_read(pci_device_t dev, u32 field);
void pci_write(pci_device_t dev, u32 field, u32 value);
i32 get_device_type(pci_device_t dev);
u32 get_secondary_bus(pci_device_t dev);
u32 pci_reach_end(pci_device_t dev);
pci_device_t pci_scan_function(u16 vendor_id, u16 device_id, u32 bus, u32 device, u32 function, i32 device_type);
pci_device_t pci_scan_device(u16 vendor_id, u16 device_id, u32 bus, u32 device, i32 device_type);
pci_device_t pci_scan_bus(u16 vendor_id, u16 device_id, u32 bus, i32 device_type);
pci_device_t pci_get_device(u16 vendor_id, u16 device_id, i32 device_type);

#endif
