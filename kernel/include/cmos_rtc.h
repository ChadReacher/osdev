#ifndef CMOS_H
#define CMOS_H

#include "types.h"

#define CMOS_COMMAND_PORT 0x70
#define CMOS_DATA_PORT 0x71

#define CMOS_REG_SECONDS   0x00
#define CMOS_REG_MINUTES   0x02
#define CMOS_REG_HOURS	   0x04
#define CMOS_REG_WEEKDAYS  0x06
#define CMOS_REG_DAY	   0x07
#define CMOS_REG_MONTH	   0x08
#define CMOS_REG_YEAR	   0x09
#define CMOS_REG_CENTURY   0x32
#define CMOS_REG_STATUS_A  0x0A
#define CMOS_REG_STATUS_B  0x0B
#define CMOS_REG_STATUS_C  0x0C

struct cmos_time {
	u8 second;
	u8 minute;
	u8 hour;
	u8 weekday;
	u8 day;
	u8 month;
	u16 year;
	u8 century;
};

void cmos_rtc_init(void);

#endif
