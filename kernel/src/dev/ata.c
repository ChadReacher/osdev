#include <ata.h>
#include <pci.h>
#include <port.h>
#include <heap.h>
#include <string.h>
#include <debug.h>
#include <paging.h>
#include <blk_dev.h>
#include <panic.h>

static pci_device_t ata_dev;
static struct hd_disk {
	u32 start_sect;
	u32 nr_sects;
} hd_disks[NR_HD*5];
static ata_device_t devices[NR_HD];

static void ata_device_init(ata_device_t *dev, u32 primary);

static void ata_io_wait(ata_device_t *dev) {
	port_inb(dev->alt_status_reg);
	port_inb(dev->alt_status_reg);
	port_inb(dev->alt_status_reg);
	port_inb(dev->alt_status_reg);
}

static void ata_software_reset(ata_device_t *dev) {
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

static void ata_device_detect(ata_device_t *dev, u32 primary) {
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
		free(dev->mem_buffer);
		free(dev->prdt);
		dev = NULL;
		DEBUG("Drive does not exist\r\n");
		return;
	}
	
	u16 lba_low = port_inb(dev->lba_low);	
	u16 lba_high = port_inb(dev->lba_high);

	if (lba_low != 0 || lba_high != 0) {
		free(dev->mem_buffer);
		free(dev->prdt);
		dev = NULL;
		DEBUG("%s", "This device is not an ATA hard disk drive\r\n");
		return;
	}

	u8 status_reg, drq_bit, err_bit;
	do {
		status_reg = port_inb(dev->status_reg);
		drq_bit = status_reg & 0x8;
		err_bit = status_reg & 0x1;
	} while (!drq_bit && !err_bit);

	if (err_bit) {
		free(dev->mem_buffer);
		free(dev->prdt);
		dev = NULL;
		DEBUG("%s", "Error occurred while polling\r\n");
		return;
	}

	for (u32 i = 0; i < 256; ++i) {
		port_inw(dev->data_reg);
	}
}

static void ata_device_init(ata_device_t *dev, u32 primary) {
	u16 data_reg = primary ? 0x1F0 : 0x170;
	u16 alt_status_reg = primary ? 0x3F6 : 0x376;

	dev->prdt = malloc(sizeof(phys_reg_desc_t));
	memset(dev->prdt, 0, sizeof(phys_reg_desc_t));
	dev->prdt_phys = (u8 *)virtual_to_physical(dev->prdt);

	dev->mem_buffer = malloc(4096);
	memset(dev->mem_buffer, 0, 4096);

	dev->prdt[0].phys_data_buf = (u32)virtual_to_physical(dev->mem_buffer);
	dev->prdt[0].transfer_size = SECTOR_SIZE;
	dev->prdt[0].mark_end = 0x8000;

	dev->data_reg = data_reg;
	dev->error_reg = data_reg + 1;
	dev->sector_count = data_reg + 2;
	dev->lba_low = data_reg + 3;
	dev->lba_mid = data_reg + 4;
	dev->lba_high = data_reg + 5;
	dev->drive_reg = data_reg + 6;
	dev->command_reg = data_reg + 7;
	dev->alt_status_reg = alt_status_reg;

	dev->bar4 = pci_read(ata_dev, PCI_BAR4);
	if (dev->bar4 & 0x1) {
		dev->bar4 = dev->bar4 & 0xFFFFFFFC;
	} else {
		dev->bar4 = dev->bar4 & 0xFFFFFFF0;
	}

	dev->bmr_command = dev->bar4;
	dev->bmr_status = dev->bar4 + 2;
	dev->bmr_prdt = dev->bar4 + 4;
}

static void ata_write(ata_device_t *dev, u32 sector, u8 nsect, i8 *buf) {
	memcpy(dev->mem_buffer, buf, SECTOR_SIZE * nsect);

	// Reset command register 
	port_outb(dev->bmr_command, 0);

	// Set the apropriate transfer size
	dev->prdt[0].transfer_size = SECTOR_SIZE * nsect;

	// Send physical PRDT address to the BMR PRDT register
	port_outl(dev->bmr_prdt, (u32)dev->prdt_phys);

	// Select the drive
	port_outb(dev->drive_reg, 0xE0 
			| (dev->slave << 4)
			| ((sector >> 24) & 0x0F));

	// Set LBA and sector count
	port_outb(dev->sector_count, nsect);
	port_outb(dev->lba_low, sector & 0xFF);
	port_outb(dev->lba_mid, (sector & 0xFF00) >> 8);
	port_outb(dev->lba_high, (sector & 0xFF0000) >> 16);

	// Send the DMA transfer(28 bit LBA) command to the ATA controller
	port_outb(dev->command_reg, 0xCA);

	// Set the Start bit on Bus Master Command Register
	port_outb(dev->bmr_command, 0x1);

	while (1) {
		u8 controller_status = port_inb(dev->bmr_status); 
		u8 drive_status = port_inb(dev->status_reg);
		if (drive_status & 0x1) {
			DEBUG("Error occured while writing to the disk\r\n");
			ata_software_reset(dev);
			return;
		}
		if (!(controller_status & 0x4)) {
			continue;
		}
		if (!(drive_status & 0x80)) {
			break;
		}
	}
}

static i8 *ata_read(ata_device_t *dev, u32 sector, u8 nsect) {
	// Reset command register
	port_outb(dev->bmr_command, 0);

	// Set the apropriate transfer size
	dev->prdt[0].transfer_size = SECTOR_SIZE * nsect;

	// Send physical PRDT address to the BMR PRDT register
	port_outl(dev->bmr_prdt, (u32)dev->prdt_phys);

	// Select the drive
	port_outb(dev->drive_reg, 0xE0 
			| (dev->slave << 4)
			| ((sector >> 24) & 0x0F));

	// Set LBA and sector count
	port_outb(dev->sector_count, nsect);
	port_outb(dev->lba_low, sector & 0xFF);
	port_outb(dev->lba_mid, (sector & 0xFF00) >> 8);
	port_outb(dev->lba_high, (sector & 0xFF0000) >> 16);

	// Send the DMA transfer(28 bit LBA) command to the ATA controller
	port_outb(dev->command_reg, 0xC8);

	// Set the Start bit on Bus Master Command Register
	port_outb(dev->bmr_command, 0x1 | 0x8);

	while (1) {
		u8 drive_status = port_inb(dev->status_reg);
		if (drive_status & 0x1) {
			DEBUG("Error occured while reading the disk\r\n");
			ata_software_reset(dev);
			return NULL;
		}
		if (drive_status & 0x8) {
			continue;
		}
		if (!(drive_status & 0x80)) {
			break;
		}
	}
	
	i8 *buf = malloc(SECTOR_SIZE * nsect);
	memcpy(buf, dev->mem_buffer, SECTOR_SIZE * nsect);
	return buf;
}

static void ata_handler(registers_state *regs) {
	(void)regs;

	port_inb(devices[0].status_reg);
	port_inb(devices[0].bmr_status);
	port_outb(devices[0].bmr_command, 0);

	port_inb(devices[1].status_reg);
	port_inb(devices[1].bmr_status);
	port_outb(devices[1].bmr_command, 0);

	// Sending EOI command code(also to the slave,
	// because IRQ14(46) > 40
	port_outb(0xA0, 0x20); // slave
	port_outb(0x20, 0x20); // master
}

void rw_ata(u32 rw, u16 dev, u32 block, i8 **buf) {
	ata_device_t *devp;
	u32 sector;

	sector = block * 2;
	if ((dev=MINOR(dev)) >= 5*NR_HD || sector+1 > hd_disks[dev].nr_sects) {
		i8 *s = (rw == READ) ? "read" : "write";
		DEBUG("Cannot %s to block %d, sector %d nr_sects: %d\r\n", s,
				block, sector, hd_disks[dev].nr_sects);
		*buf = NULL;
		return;
	}
	sector += hd_disks[dev].start_sect;
	dev /= 5;
	devp = &devices[dev];
	if (rw == READ) {
		*buf = ata_read(devp, sector, 2);
	} else if (rw == WRITE) {
		ata_write(devp, sector, 2, *buf);
	}
}

void ata_init() {
	ata_dev = pci_get_device(ATA_PCI_VENDOR_ID, ATA_PCI_DEVICE_ID, -1);
	
	register_interrupt_handler(IRQ14, ata_handler);
	register_interrupt_handler(IRQ15, ata_handler);

	u32 pci_command_reg = pci_read(ata_dev, PCI_COMMAND);
	if (!(pci_command_reg & (1 << 2))) {
		pci_command_reg |= (1 << 2);
		pci_write(ata_dev, PCI_COMMAND, pci_command_reg);
	}

	devices[0].slave = 0;
	devices[1].slave = 1;
	ata_device_detect(&devices[0], 1);
	ata_device_detect(&devices[1], 1);

	i8 *boot_sect;
	struct partition *p;
	for (u32 drive = 0; drive < NR_HD; ++drive) {
		boot_sect = ata_read(&devices[drive], 0, 1);
		if (boot_sect[510] != 0x55 && (u8)boot_sect[511] != 0xAA) {
			kernel_panic("Bad partition table on drive %d\n", drive);
		}
		p = (struct partition *)(boot_sect + 0x1BE);
		for (u32 i = 1; i < 5; ++i, ++p) {
			hd_disks[i + 5 * drive].start_sect = p->start_sect;
			hd_disks[i + 5 * drive].nr_sects = p->nr_sects;
			DEBUG("Drive #%d, partition #%d: start_sect - %d, nr_sects - %d\r\n", 
					drive, i, p->start_sect, p->nr_sects);
		}
		free(boot_sect);
	}
}
