#include "ata.h"
#include "pci.h"
#include "port.h"
#include "heap.h"
#include "memory.h"
#include "string.h"
#include "debug.h"
#include "paging.h"

pci_device_t ata_device;

ata_device_t primary_master = {.slave = 0};
ata_device_t primary_slave = {.slave = 1};
ata_device_t secondary_master = {.slave = 0};
ata_device_t secondary_slave = {.slave = 1};

void ata_init() {
	ata_device = pci_get_device(ATA_VENDOR_ID, ATA_DEVICE_ID, -1);
	
	register_interrupt_handler(IRQ14, ata_handler);

	ata_device_detect(&primary_master, 1);
	ata_device_detect(&primary_slave, 1);
	ata_device_detect(&secondary_master, 0);
	ata_device_detect(&secondary_slave, 0);
}

void ata_device_detect(ata_device_t *dev, u32 primary) {
	ata_device_init(dev, primary);

	ata_software_reset(dev);
	ata_io_wait(dev);

	port_outb(dev->drive_reg, (0xA + dev->slave) << 4);

	port_outb(dev->sector_count, 0);
	port_outb(dev->lba_low, 0);
	port_outb(dev->lba_mid, 0);
	port_outb(dev->lba_high, 0);
	port_outb(dev->command_reg, IDENTIFY_COMMAND);
	u8 res = port_inb(dev->status_reg);
	if (res == 0) {
		DEBUG("Drive %s does not exist\r\n", dev->mountpoint);
		return;
	}
	
	u16 lba_low = port_inb(dev->lba_low);	
	u16 lba_high = port_inb(dev->lba_high);

	DEBUG("lba_low = %x, lba_high = %x\r\n", lba_low, lba_high);
	if (lba_low == 0x14 && lba_high == 0xEB) {
		DEBUG("%s", "ATADEV_PATAPI\r\n");
	} else if (lba_low == 0x69 && lba_high == 0x96) {
		DEBUG("%s", "ATADEV_SATAPI\r\n");
	} else if (lba_low == 0x0 && lba_high == 0x0) {
		DEBUG("%s", "ATADEV_PATA\r\n");
	} else if (lba_low == 0x3C && lba_high == 0xC3) {
		DEBUG("%s", "ATADEV_SATA\r\n");
	}

	if (lba_low != 0 || lba_high != 0) {
		DEBUG("%s", "This device is not an ATA hard disk drive\r\n");
		return;
	}

	u8 drq_bit, err_bit;
	do {
		drq_bit = port_inb(dev->status_reg) & 0x8;
		err_bit = port_inb(dev->status_reg) & 0x0;
	} while (!drq_bit && !err_bit);

	if (err_bit) {
		DEBUG("%s", "Error occurred while polling\r\n");
		return;
	}

	for (u32 i = 0; i < 256; ++i) {
		port_inw(dev->data_reg);
	}

	u32 pci_command_reg = pci_read(ata_device, PCI_COMMAND);
	if (!(pci_command_reg & (1 << 2))) {
		pci_command_reg |= (1 << 2);
		pci_write(ata_device, PCI_COMMAND, pci_command_reg);
	}

	vfs_node_t *vfs_node_ata_device = ata_create_device(dev);
	DEBUG("%s", "Created vfs node for ata device\r\n");
	vfs_mount(dev->mountpoint, vfs_node_ata_device);
}

void ata_device_init(ata_device_t *dev, u32 primary) {
	u16 data_reg = primary ? 0x1F0 : 0x170;
	u16 alt_status_reg = primary ? 0x3F6 : 0x376;

	dev->prdt = malloc(sizeof(phys_reg_desc_t));
	memset(dev->prdt, 0, sizeof(phys_reg_desc_t));
	dev->prdt_phys = (u8 *)virtual_to_physical(dev->prdt);

	dev->mem_buffer = malloc(4096);
	memset(dev->mem_buffer, 0, 4096);

	dev->prdt[0].phys_data_buf = (u32)virtual_to_physical(dev->mem_buffer);
	dev->prdt[0].transfer_size = SECTOR_SIZE;
	dev->prdt[0].mark_end = 1;

	dev->data_reg = data_reg;
	dev->error_reg = data_reg + 1;
	dev->sector_count = data_reg + 2;
	dev->lba_low = data_reg + 3;
	dev->lba_mid = data_reg + 4;
	dev->lba_high = data_reg + 5;
	dev->drive_reg = data_reg + 6;
	dev->command_reg = data_reg + 7;
	dev->alt_status_reg = alt_status_reg;

	dev->bar4 = pci_read(ata_device, PCI_BAR4);
	if (dev->bar4 & 0x1) {
		dev->bar4 = dev->bar4 & 0xFFFFFFFC;
	} else {
		dev->bar4 = dev->bar4 & 0xFFFFFFF0;
	}

	dev->bmr_command = dev->bar4;
	dev->bmr_status = dev->bar4 + 2;
	dev->bmr_prdt = dev->bar4 + 4;

	memset(dev->mountpoint, 0, 32);
	strcpy(dev->mountpoint, "/dev/hd");
	dev->mountpoint[strlen(dev->mountpoint)] = 'a' + (((!primary) << 1) | dev->slave);
	DEBUG("Device mountpoint - %s\r\n", dev->mountpoint);
}

void ata_software_reset(ata_device_t *dev) {
	port_outb(dev->control_reg, 1 << 2);
	port_outb(dev->control_reg, 0);
	ata_io_wait(dev);
	u8 control_reg_val;
	u32 count = 1000000;
	do {
		--count;
		control_reg_val = port_inb(dev->control_reg);
		if (count == 0) {
			return;
		}
	} while ((control_reg_val & 0xC0) != 0x40);
}

void ata_io_wait(ata_device_t *dev) {
	port_inb(dev->alt_status_reg);
	port_inb(dev->alt_status_reg);
	port_inb(dev->alt_status_reg);
	port_inb(dev->alt_status_reg);
}

void ata_write_sector(ata_device_t *dev, u32 lba, i8 *buf) {
	memcpy(dev->mem_buffer, buf, SECTOR_SIZE);

	port_outb(dev->bmr_command, 0);

	//Send physical PRDT address to the BMR PRDT register
	port_outl(dev->bmr_prdt, (u32)dev->prdt_phys);

	// Select the drive
	port_outb(dev->drive_reg, 0xE0 | dev->slave << 4 | (lba & 0x0F000000) >> 24);

	// Set LBA and sector count
	port_outb(dev->sector_count, 1);
	port_outb(dev->lba_low, lba & 0xFF);
	port_outb(dev->lba_mid, (lba & 0xFF00) >> 8);
	port_outb(dev->lba_high, (lba & 0xFF0000) >> 16);

	// Send the DMA transfer(28 bit LBA) command to the ATA controller
	port_outb(dev->command_reg, 0xCA);

	// Set the Start bit on Bus Master Command Register
	port_outb(dev->bmr_command, 0x1);

	while (1) {
		u8 controller_status = port_inb(dev->bmr_status); 
		u8 drive_status = port_inb(dev->status_reg);
		if (!(controller_status & 0x4)) {
			continue;
		}
		if (!(drive_status & 0x80)) {
			break;
		}
	}
}

i8 *ata_read_sector(ata_device_t *dev, u32 lba) {
	i8 *buf = malloc(SECTOR_SIZE);

	port_outb(dev->bmr_command, 0);

	//Send physical PRDT address to the BMR PRDT register
	port_outl(dev->bmr_prdt, (u32)dev->prdt_phys);

	// Select the drive
	port_outb(dev->drive_reg, 0xE0 | dev->slave << 4 | (lba & 0x0F000000) >> 24);

	// Set LBA and sector count
	port_outb(dev->sector_count, 1);
	port_outb(dev->lba_low, lba & 0xFF);
	port_outb(dev->lba_mid, (lba & 0xFF00) >> 8);
	port_outb(dev->lba_high, (lba & 0xFF0000) >> 16);

	// Send the DMA transfer(28 bit LBA) command to the ATA controller
	port_outb(dev->command_reg, 0xC8);

	// Set the Start bit on Bus Master Command Register
	port_outb(dev->bmr_command, 0x1 | 0x8);

	while (1) {
		u8 drive_status = port_inb(dev->status_reg);
		if (drive_status & 0x8) {
			continue;
		}
		if (!(drive_status & 0x80)) {
			break;
		}
	}
	
	memcpy(buf, dev->mem_buffer, SECTOR_SIZE);	
	return buf;
}

u32 ata_read(vfs_node_t *node, u32 offset, u32 size, i8 *buf) {
	u32 start_sector = offset / SECTOR_SIZE;
	u32 start_sector_offset = offset % SECTOR_SIZE;

	u32 end_sector = (offset + size - 1) / SECTOR_SIZE;
	u32 end_sector_offset = (offset + size - 1) % SECTOR_SIZE;

	u32 sector_counter = start_sector;
	u32 read_size;
	u32 buf_off, total_read_bytes = 0;

	while (sector_counter <= end_sector) {
		buf_off = 0;
		read_size = SECTOR_SIZE;

		i8 *ret_buf = ata_read_sector((ata_device_t*)node->device, sector_counter);

		if (sector_counter == start_sector) {
			buf_off = start_sector_offset;
			read_size = SECTOR_SIZE - buf_off;
		} 
		
		if (sector_counter == end_sector) {
			read_size = end_sector_offset - buf_off + 1;
		}
		
		memcpy(buf, ret_buf + buf_off, read_size);
		buf += read_size;
		total_read_bytes += read_size;
		++sector_counter;

		free(ret_buf);
	}
	return total_read_bytes;
}

u32 ata_write(vfs_node_t *node, u32 offset, u32 size, i8 *buf) {
	u32 start_sector = offset / SECTOR_SIZE;
	u32 start_sector_offset = offset % SECTOR_SIZE;

	u32 end_sector = (offset + size - 1) / SECTOR_SIZE;
	u32 end_sector_offset = (offset + size - 1) % SECTOR_SIZE;

	u32 sector_counter = start_sector;
	u32 write_size;
	u32 buf_offset, total_read_bytes = 0;

	while (sector_counter <= end_sector) {
		buf_offset = 0;
		write_size = SECTOR_SIZE;

		i8 *ret_buf = ata_read_sector((ata_device_t*)node->device, sector_counter);

		if (sector_counter == start_sector) {
			buf_offset = start_sector_offset;
			write_size = SECTOR_SIZE - buf_offset;
		} 
		
		if (sector_counter == end_sector) {
			write_size = end_sector_offset - buf_offset + 1;
		}
		
		memcpy(ret_buf + buf_offset, buf, write_size);
		ata_write_sector((ata_device_t*)node->device, sector_counter, ret_buf);
		buf += write_size;
		total_read_bytes += write_size;
		++sector_counter;
	}
	return total_read_bytes;
}

u32 ata_open(vfs_node_t *node, u32 flags) {
	(void)node;
	(void)flags;
	return 0;
}

u32 ata_close(vfs_node_t *node) {
	(void)node;
	return 0;
}

void ata_handler(registers_state regs) {
	(void)regs;

	port_inb(primary_master.status_reg);
	port_inb(primary_master.bmr_status);
	port_outb(primary_master.bmr_command, 0);

	// Sending EOI command code(also to the slave,
	// because IRQ14(46) > 40
	port_outb(0xA0, 0x20); // slave
	port_outb(0x20, 0x20); // master
}

vfs_node_t *ata_create_device(ata_device_t *dev) {
	vfs_node_t *ata_dev_vfs_node = malloc(sizeof(vfs_node_t));
	i8 *name = dev->mountpoint + strlen(dev->mountpoint) - 1 - 2;
	strcpy(ata_dev_vfs_node->name, name);
	
	/*
	strcpy(ata_dev_vfs_node->name, "ata device ");
	u32 name_len = strlen(ata_dev_vfs_node->name);
	ata_dev_vfs_node->name[name_len] = dev->mountpoint[strlen(dev->mountpoint) - 1];
	ata_dev_vfs_node->name[name_len + 1] = '\0';
	*/
	DEBUG("Name of vfs node - %s\r\n", ata_dev_vfs_node->name);
	ata_dev_vfs_node->flags = FS_BLOCKDEVICE;
	ata_dev_vfs_node->read = ata_read;
	ata_dev_vfs_node->write = ata_write;
	ata_dev_vfs_node->open = ata_open;
	ata_dev_vfs_node->close = ata_close;
	ata_dev_vfs_node->device = dev;

	return ata_dev_vfs_node;
}

