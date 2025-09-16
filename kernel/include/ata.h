#ifndef ATA_H
#define ATA_H

#include "types.h"
#include <paging.h>

struct phys_reg_desc {
	u32 phys_data_buf;
	u16 transfer_size;
	u16 mark_end;
} __attribute__((packed));

struct ata_device {
	u16 data_reg;
	u16 error_reg;
	u16 sector_count;
	u16 lba_low;
	u16 lba_mid;
	u16 lba_high;
	u16 drive_reg;
	u16 status_reg;
	u16 command_reg;
	u16 alt_status_reg;
	u16 control_reg;
	i32 slave;
	u32 bar4;
	u32 bmr_command;
	u32 bmr_status;
	u32 bmr_prdt;

	struct phys_reg_desc *prdt;
	physical_address prdt_phys;
	u8 *mem_buffer;
};

struct partition {
	u8 boot_ind;
	u8 head;
	u8 sector;
	u8 cyl;
	u8 sys_id;
	u8 end_head;
	u8 end_sector;
	u8 end_cyl;
	u32 start_sect;
	u32 nr_sects;
};

#define ATA_PCI_VENDOR_ID 0x8086
#define ATA_PCI_DEVICE_ID 0x7010

/* For now supporting only 2 drives */
#define NR_HD 2
#define IDENTIFY_COMMAND 0xEC
#define WRITE_DMA_COMMAND 0xCA
#define READ_DMA_COMMAND 0xC8

#define SECTOR_SIZE 512

#define PRIMARY_DATA_REG 0x1F0
#define SECONDARY_DATA_REG 0x170
#define PRIMARY_ALT_STATUS_REG 0x3F6
#define SECONDARY_ALT_STATUS_REG 0x376

#define STATUS_ERR  0x01
#define STATUS_DRQ  0x08
#define STATUS_DRDY 0x40
#define STATUS_BSY  0x80

#define CONTROL_NIEN    0x02
#define CONTROL_SRST    0x04
#define CONTROL_HOB     0x80

void ata_init(void);
void rw_ata(u32 rw, u16 dev, u32 block, i8 *buf);

u32 dev_by_path(const i8 *device);

#endif
