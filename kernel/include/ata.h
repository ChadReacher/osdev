#ifndef ATA_H
#define ATA_H

#include "types.h"
#include "isr.h"

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
} ata_device_t;

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

// For now supporting only 2 drives
#define NR_HD 2
#define IDENTIFY_COMMAND 0xEC

#define SECTOR_SIZE 512

void ata_init();
void rw_ata(u32 rw, u16 dev, u32 block, i8 **buf);

#endif
