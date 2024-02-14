#ifndef ATA_H
#define ATA_H

#include "types.h"
#include "isr.h"
#include "vfs.h"

typedef struct phys_reg_desc {
	u32 phys_data_buf;
	u16 transfer_size;
	u16 mark_end;
} __attribute__((packed)) phys_reg_desc_t;

typedef struct ata_device {
	u16 data_reg;
	u16 error_reg;
	u16 sector_count;
	union {
		u16 sector_num;
		u16 lba_low;
	};
	union {
		u16 cylinder_low;
		u16 lba_mid;
	};
	union {
		u16 cylinder_high;
		u16 lba_high;
	};
	union {
		u16 drive_reg;
		u16 head_reg;
	};
	union {
		u16 status_reg;
		u16 command_reg;
	};
	union {
		u16 alt_status_reg;
		u16 control_reg;
	};
	i32 slave;
	u32 bar4;
	u32 bmr_command;
	u32 bmr_status;
	u32 bmr_prdt;

	phys_reg_desc_t *prdt;
	u8 *prdt_phys;

	u8 *mem_buffer;

	i8 mountpoint[32];

} __attribute__((packed)) ata_device_t;

// PCI characterists
#define ATA_VENDOR_ID 0x8086
#define ATA_DEVICE_ID 0x7010

#define IDENTIFY_COMMAND 0xEC

#define SECTOR_SIZE 512

void ata_init();
void ata_handler(registers_state *regs);
void ata_software_reset(ata_device_t *dev);
void ata_io_wait(ata_device_t *dev);
void ata_device_detect(ata_device_t *dev, u32 primary);
void ata_device_init(ata_device_t *dev, u32 primary);
vfs_node_t *ata_create_device(ata_device_t *dev);
i8 *ata_read_sector(ata_device_t *dev, u32 lba);
void ata_write_sector(ata_device_t *dev, u32 lba, i8 *buf);

#endif
