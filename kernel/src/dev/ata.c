#include <ata.h>
#include <pci.h>
#include <port.h>
#include <heap.h>
#include <string.h>
#include <paging.h>
#include <blk_dev.h>
#include <panic.h>

static union pci_device ata_dev;
static struct hd_disk {
	u32 start_sect;
	u32 nr_sects;
} hd_disks[NR_HD*5];
static struct ata_device devices[NR_HD];

static void ata_device_init(struct ata_device *dev, u32 primary);

static void ata_io_wait(struct ata_device *dev) {
	port_inb(dev->alt_status_reg);
	port_inb(dev->alt_status_reg);
	port_inb(dev->alt_status_reg);
	port_inb(dev->alt_status_reg);
}

static void ata_software_reset(struct ata_device *dev) {
	u8 control_reg_val;
	u32 count;

	port_outb(dev->control_reg, 1 << 2);
	port_outb(dev->control_reg, 0);
	ata_io_wait(dev);
	count = 1000000;
	do {
		--count;
		control_reg_val = port_inb(dev->control_reg);
		if (count == 0) {
			return;
		}
	} while ((control_reg_val & 0xC0) != 0x40);
}

static void ata_device_detect(struct ata_device *dev, u32 primary) {
	u8 res, status_reg, drq_bit, err_bit;
	u16 lba_low, lba_high;
	u32 i;

	ata_device_init(dev, primary);

	ata_software_reset(dev);
	ata_io_wait(dev);

	port_outb(dev->drive_reg, (0xA + dev->slave) << 4);

	port_outb(dev->sector_count, 0);
	port_outb(dev->lba_low, 0);
	port_outb(dev->lba_mid, 0);
	port_outb(dev->lba_high, 0);
	port_outb(dev->command_reg, IDENTIFY_COMMAND);
	res = port_inb(dev->status_reg);
	if (res == 0) {
		free(dev->mem_buffer);
		free(dev->prdt);
		dev = NULL;
		debug("Drive does not exist\r\n");
		return;
	}
	
	lba_low = port_inb(dev->lba_low);	
	lba_high = port_inb(dev->lba_high);

	if (lba_low != 0 || lba_high != 0) {
		free(dev->mem_buffer);
		free(dev->prdt);
		dev = NULL;
		debug("This device is not an ATA hard disk drive\r\n");
		return;
	}

	do {
		status_reg = port_inb(dev->status_reg);
		drq_bit = status_reg & 0x8;
		err_bit = status_reg & 0x1;
	} while (!drq_bit && !err_bit);

	if (err_bit) {
		free(dev->mem_buffer);
		free(dev->prdt);
		dev = NULL;
		debug("Error occurred while polling\r\n");
		return;
	}

	for (i = 0; i < 256; ++i) {
		port_inw(dev->data_reg);
	}
}

extern u32 prdt_struct;
extern u32 prdt_mem_buffer;

static void ata_device_init(struct ata_device *dev, u32 primary) {
	u16 data_reg = primary ? 0x1F0 : 0x170;
	u16 alt_status_reg = primary ? 0x3F6 : 0x376;

        dev->prdt = (struct phys_reg_desc *)&prdt_struct;
	memset(dev->prdt, 0, sizeof(struct phys_reg_desc));
	dev->prdt_phys = (u8 *)virtual_to_physical(dev->prdt);

	dev->mem_buffer = (u8 *)(&prdt_mem_buffer);
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
	dev->status_reg = dev->command_reg = data_reg + 7;
	dev->control_reg = dev->alt_status_reg = alt_status_reg;

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

static void ata_write(struct ata_device *dev, u32 sector, u8 nsect, i8 *buf) {
	/* Set the apropriate transfer size */
	dev->prdt[0].transfer_size = SECTOR_SIZE * nsect;

	/* Send physical PRDT address to the BMR PRDT register */
	port_outl(dev->bmr_prdt, (u32)dev->prdt_phys);

        /* Do some setup for controller status. Don't understand any of it :( */
        u8 controller_status = port_inb(dev->bmr_status);
#define BMR_STATUS_ERROR        (1 << 1)
#define BMR_STATUS_IRQ          (1 << 2)
        controller_status |= BMR_STATUS_ERROR | BMR_STATUS_IRQ;
        port_outb(dev->bmr_status, controller_status);

        /* Transfer the buffer to the PRDT buffer location */
	memcpy(dev->mem_buffer, buf, SECTOR_SIZE * nsect);

	/* Reset command register */
	port_outb(dev->bmr_command, 0);

        /* Wait for a little bit ... */
        while (1) {
                u8 drive_status = port_inb(dev->status_reg);
		if ((drive_status & 0x80) == 0) {
			break;
		}
        }

	/* Select the drive */
	port_outb(dev->drive_reg, 0xE0 
			| (dev->slave << 4)
			| ((sector >> 24) & 0x0F));
        ata_io_wait(dev);

	/* Set LBA and sector count */
	port_outb(dev->sector_count, nsect);
	port_outb(dev->lba_low, sector & 0xFF);
	port_outb(dev->lba_mid, (sector & 0xFF00) >> 8);
	port_outb(dev->lba_high, (sector & 0xFF0000) >> 16);

        while (1) {
		u8 drive_status = port_inb(dev->status_reg);
		if ( ((drive_status & 0x80) == 0) && (drive_status & 0x40) ) {
			break;
		}
        }

	/* Send the DMA transfer(28 bit LBA) command to the ATA controller */
	port_outb(dev->command_reg, 0xCA);

        ata_io_wait(dev);

	/* Set the Start bit on Bus Master Command Register */
	port_outb(dev->bmr_command, 0x1);

	while (1) {
		controller_status = port_inb(dev->bmr_status); 
		u8 drive_status = port_inb(dev->status_reg);
		if (!(controller_status & 0x4)) {
			continue;
		}
                if (controller_status & 0x2) {
                        panic("DMA error\r\n");
                }
		if (drive_status & 0x1) {
			panic("Error occured while writing to the disk\r\n");
			ata_software_reset(dev);
			return;
		}
		if (!(drive_status & 0x80)) {
			break;
		}
	}
        u8 drive_status = port_inb(dev->status_reg);
        if ((drive_status & 1) == 0) {
                if (controller_status & 0x2) {
                        panic("B");
                }
        } else {
                panic("A");
        }
        u8 bmr_command = port_inb(dev->bmr_command);
        port_outb(dev->bmr_command, bmr_command & ~0x1);
        u8 bmr_status = port_inb(dev->bmr_status);
        port_outb(dev->bmr_status, bmr_status | 0x2 | 0x4);
}

static void ata_read(struct ata_device *dev, u32 sector, u8 nsect, i8 *buf) {
	/* Reset command register */
	port_outb(dev->bmr_command, 0);

	/* Set the apropriate transfer size */
	dev->prdt[0].transfer_size = SECTOR_SIZE * nsect;

	/* Send physical PRDT address to the BMR PRDT register */
	port_outl(dev->bmr_prdt, (u32)dev->prdt_phys);

	/* Select the drive */
	port_outb(dev->drive_reg, 0xE0 
			| (dev->slave << 4)
			| ((sector >> 24) & 0x0F));

	/* Set LBA and sector count */
	port_outb(dev->sector_count, nsect);
	port_outb(dev->lba_low, sector & 0xFF);
	port_outb(dev->lba_mid, (sector & 0xFF00) >> 8);
	port_outb(dev->lba_high, (sector & 0xFF0000) >> 16);

	/* Send the DMA transfer(28 bit LBA) command to the ATA controller */
	port_outb(dev->command_reg, 0xC8);

	/* Set the Start bit on Bus Master Command Register */
	port_outb(dev->bmr_command, 0x1 | 0x8);

	while (1) {
		u8 drive_status = port_inb(dev->status_reg);
		if (drive_status & 0x1) {
			debug("Error occured while reading the disk\r\n");
			ata_software_reset(dev);
			return;
		}
		if (drive_status & 0x8) {
			continue;
		}
		if (!(drive_status & 0x80)) {
			break;
		}
	}
	
	memcpy(buf, dev->mem_buffer, SECTOR_SIZE * nsect);
}

static void ata_handler(struct registers_state *regs) {
	(void)regs;

	port_inb(devices[0].status_reg);
	port_inb(devices[0].bmr_status);
	port_outb(devices[0].bmr_command, 0);

	port_inb(devices[1].status_reg);
	port_inb(devices[1].bmr_status);
	port_outb(devices[1].bmr_command, 0);

	/* Sending EOI command code(also to the slave, */
	/* because IRQ14(46) > 40 */
	port_outb(0xA0, 0x20); /* slave */
	port_outb(0x20, 0x20); /* master */
}

void rw_ata(u32 rw, u16 dev, u32 block, i8 *buf) {
	struct ata_device *devp;

	u32 sector = block * 2;
        dev = MINOR(dev);
	if (dev >= 5 * NR_HD || sector + 1 > hd_disks[dev].nr_sects) {
		i8 *s = (rw == READ) ? "read" : "write";
		debug("Cannot %s to block %d, sector %d nr_sects: %d\r\n", s,
				block, sector, hd_disks[dev].nr_sects);
		return;
	}
	sector += hd_disks[dev].start_sect;
	dev /= 5;
	devp = &devices[dev];
	if (rw == READ) {
		ata_read(devp, sector, 2, buf);
	} else if (rw == WRITE) {
		ata_write(devp, sector, 2, buf);
	}
}

void ata_init(void) {
	i8 boot_sect[512];
	struct partition *p;
	u32 drive, i, pci_command_reg;

	ata_dev = pci_get_device(ATA_PCI_VENDOR_ID, ATA_PCI_DEVICE_ID, -1);
	
	register_interrupt_handler(IRQ14, ata_handler);
	register_interrupt_handler(IRQ15, ata_handler);

        // Access command registers of the device,
        // enables the device to act as the bus master.
        // This basically enables DMA.
	pci_command_reg = pci_read(ata_dev, PCI_COMMAND);
	if (!(pci_command_reg & (1 << 2))) {
		pci_command_reg |= (1 << 2);
		pci_write(ata_dev, PCI_COMMAND, pci_command_reg);
	}

	devices[0].slave = 0;
	devices[1].slave = 1;
	ata_device_detect(&devices[0], 1);
	ata_device_detect(&devices[1], 1);

	for (drive = 0; drive < NR_HD; ++drive) {
		ata_read(&devices[drive], 0, 1, boot_sect);
		if (boot_sect[510] != 0x55 && (u8)boot_sect[511] != 0xAA) {
			panic("Bad partition table on drive %d\n", drive);
		}
		p = (struct partition *)(boot_sect + 0x1BE);
		for (i = 1; i < 5; ++i, ++p) {
			hd_disks[i + 5 * drive].start_sect = p->start_sect;
			hd_disks[i + 5 * drive].nr_sects = p->nr_sects;
			debug("DRIVE #%d partition #%d: "
				  "start_sect - %d, nr_sects - %d\r\n",
				  drive, i, p->start_sect, p->nr_sects);
		}
	}
}
