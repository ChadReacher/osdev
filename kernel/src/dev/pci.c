#include <pci.h>
#include <port.h>
#include <panic.h>

u32 pci_size_map[100];
pci_device_t dev_zero = {0};

void pci_init() {
	pci_device_t dev = {0};
	u32 bus, device, function;

	pci_size_map[PCI_VENDOR_ID] = 2;
	pci_size_map[PCI_DEVICE_ID] = 2;
	pci_size_map[PCI_COMMAND] = 2;
	pci_size_map[PCI_STATUS] = 2;
	pci_size_map[PCI_REVISION_ID] = 1;
	pci_size_map[PCI_PROG_IF] = 1;
	pci_size_map[PCI_SUBCLASS] = 1;
	pci_size_map[PCI_CLASS] = 1;
	pci_size_map[PCI_CACHE_LINE_SIZE] = 1;
	pci_size_map[PCI_LATENCY_TIMER] = 1;
	pci_size_map[PCI_HEADER_TYPE] = 1;
	pci_size_map[PCI_BIST] = 1;
	pci_size_map[PCI_BAR0] = 4;
	pci_size_map[PCI_BAR1] = 4;
	pci_size_map[PCI_BAR2] = 4;
	pci_size_map[PCI_BAR3] = 4;
	pci_size_map[PCI_BAR4] = 4;
    pci_size_map[PCI_BAR5] = 5;
	pci_size_map[PCI_INTERRUPT_LINE] = 1;
	pci_size_map[PCI_SECONDARY_BUS] = 1;

	/* Leave it for now just to test PCI functions */
	for (bus = 0; bus < 256; ++bus) {
		dev.s.bus_num = bus;
		for (device = 0; device < 32; ++device) {
			dev.s.device_num = device;
			for (function = 0; function < 8; ++function) {
				u32 vendor_id = pci_read(dev, PCI_VENDOR_ID);
				u32 device_id = pci_read(dev, PCI_DEVICE_ID);
				u32 class = pci_read(dev, PCI_CLASS);
				u32 subclass = pci_read(dev, PCI_SUBCLASS);
				dev.s.function_num = function;
				if (vendor_id != 0xFFFF && device_id != 0xFFFF) {
					debug("Found a PCI device with device_id = 0x%x and vendor_id = 0x%x\r\n", device_id, vendor_id);
					debug("Class - 0x%x, Subclass - 0x%x\r\n", class, subclass);
				}
			}
		}
	}
	dev = pci_get_device(0x8086, 0x7010, -1);
	debug("Found dev - 0x%x\r\n", dev.bits);
}

void pci_write(pci_device_t dev, u32 field, u32 value) {
	dev.s.field_num = (field & 0xFC) >> 2;
	dev.s.enable_bit = 1;
	port_outl(PCI_CONFIG_ADDRESS, dev.bits);
	port_outl(PCI_CONFIG_DATA, value);
}

u32 pci_read(pci_device_t dev, u32 field) {
	u32 field_size;

	dev.s.enable_bit = 1;
	dev.s.field_num = (field & 0xFC) >> 2;
	port_outl(PCI_CONFIG_ADDRESS, dev.bits);

	field_size = pci_size_map[field];
	if (field_size == 1) {
		u8 res = port_inb(PCI_CONFIG_DATA + (field & 3));
		return res;
	} else if (field_size == 2) {
		u16 res = port_inw(PCI_CONFIG_DATA + (field & 2));
		return res;
	} else if (field_size == 4) {
		u32 res = port_inl(PCI_CONFIG_DATA);
		return res;
	}
	return 0xFFFF;
}

i32 get_device_type(pci_device_t dev) {
	i32 dev_type = pci_read(dev, PCI_CLASS) << 8;
	dev_type |= pci_read(dev, PCI_SUBCLASS);
	return dev_type;
}

u32 get_secondary_bus(pci_device_t dev) {
	return pci_read(dev, PCI_SECONDARY_BUS);
}

u32 pci_reach_end(pci_device_t dev) {
	u32 header_type = pci_read(dev, PCI_HEADER_TYPE);
	return !header_type;
}

pci_device_t pci_scan_function(u16 vendor_id, u16 device_id, u32 bus, u32 device, u32 function, i32 device_type) {
	pci_device_t dev = {0};
	dev.s.function_num = function;
	dev.s.device_num = device;
	dev.s.bus_num = bus;

	if (get_device_type(dev) == PCI_TYPE_BRIDGE) {
		pci_scan_bus(vendor_id, device_id, get_secondary_bus(dev), device_type);
	}

	if (device_type == -1 || device_type == get_device_type(dev)) {
		u32 dev_id = pci_read(dev, PCI_DEVICE_ID);
		u32 ven_id = pci_read(dev, PCI_VENDOR_ID);
		if (dev_id == device_id && ven_id == vendor_id) {
			return dev;
		}
	}
	return dev_zero;
}

pci_device_t pci_scan_device(u16 vendor_id, u16 device_id, u32 bus, u32 device, i32 device_type) {
	u32 function;
	pci_device_t dev = {0};
	pci_device_t t;

	dev.s.bus_num = bus;
	dev.s.device_num = device;
	if (pci_read(dev, PCI_VENDOR_ID) == PCI_NONE) {
		return dev_zero;
	}

	t = pci_scan_function(vendor_id, device_id, bus, device, 0, device_type);
	if (t.bits) {
		return t;
	}

	if (pci_reach_end(dev)) {
		return dev_zero;
	}

	for (function = 1; function < FUNCTION_PER_DEVICE; ++function) {
		if (pci_read(dev, PCI_VENDOR_ID) != PCI_NONE) {
			t = pci_scan_function(vendor_id, device_id, bus, device, function, device_type);
			if (t.bits) {
				return t;
			}
		}
	}

	return dev_zero;
}

pci_device_t pci_scan_bus(u16 vendor_id, u16 device_id, u32 bus, i32 device_type) {
	u32 device;
	for (device = 0; device < DEVICE_PER_BUS; ++device) {
		pci_device_t dev = pci_scan_device(vendor_id, device_id, bus, device, device_type);
		if (dev.bits) {
			return dev;
		}
	}
	return dev_zero;
}

pci_device_t pci_get_device(u16 vendor_id, u16 device_id, i32 device_type) {	
	u32 function;
	pci_device_t dev = pci_scan_bus(vendor_id, device_id, 0, device_type);
	if (dev.bits) {
		return dev;
	}

	if (pci_reach_end(dev_zero)) {
		debug("%s", "pci_get_device() failed\r\n");
	}

	for (function = 1; function < FUNCTION_PER_DEVICE; ++function) {
		pci_device_t dev = {0};
		dev.s.function_num = function;
		if (pci_read(dev, PCI_VENDOR_ID) == PCI_NONE) {
			break;
		}
		dev = pci_scan_bus(vendor_id, device_id, function, device_type);
		if (dev.bits) {
			return dev;
		}
	}
	return dev_zero;
}
