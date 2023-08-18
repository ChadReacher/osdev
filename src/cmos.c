#include "cmos.h"
#include "string.h"
#include "port.h"
#include "debug.h"

bool rtc_values_are_not_equal(cmos_rtc_t left, cmos_rtc_t right) {
	return (
		left.seconds != right.seconds ||
		left.minutes != right.minutes ||
		left.hours != right.hours ||
		left.weekdays != right.weekdays ||
		left.day != right.day ||
		left.month != right.month ||
		left.year != right.year ||
		left.century != right.century
	);
}

u8 cmos_read_register(u8 reg) {
	// To read a register, first you need to select it
	// Also you should disable NMI by setting 0x80 bit
	// After that you can read the value of the register
	// on CMOS_DATA_PORT(0x71)
	port_outb(CMOS_COMMAND_PORT, (1 << 7 | reg));
	return port_inb(CMOS_DATA_PORT);
}

bool cmos_update_in_progress() {
	port_outb(CMOS_COMMAND_PORT, CMOS_REG_STATUS_A);
	// The RTC has an "Update in progress" flag (bit 7 of Status Register A)
	return port_inb(CMOS_DATA_PORT) & 0x80;
}

cmos_rtc_t cmos_read_rtc() {
	// First, you have to ensure that you won't be effected
	// by an update. Then select the associated "CMOS register"
	// and read value 
	cmos_rtc_t rtc;
	cmos_rtc_t last;

	// This uses the "read registers until you get the same values twice in a
	// row" technique to avoid getting inconsistent values due to RTC updates
	while (cmos_update_in_progress());

	// Read first time
	rtc.seconds = cmos_read_register(CMOS_REG_SECONDS);
	rtc.minutes = cmos_read_register(CMOS_REG_MINUTES);
	rtc.hours = cmos_read_register(CMOS_REG_HOURS);
	rtc.weekdays = cmos_read_register(CMOS_REG_WEEKDAYS);
	rtc.day = cmos_read_register(CMOS_REG_DAY);
	rtc.month = cmos_read_register(CMOS_REG_MONTH);
	rtc.year = cmos_read_register(CMOS_REG_YEAR);
	rtc.century = cmos_read_register(CMOS_REG_CENTURY);

	do {
		// Prepare to read for a second time
		memcpy(&last, &rtc, sizeof(cmos_rtc_t));

		while (cmos_update_in_progress());

		// Read second time
		rtc.seconds = cmos_read_register(CMOS_REG_SECONDS);
		rtc.minutes = cmos_read_register(CMOS_REG_MINUTES);
		rtc.hours = cmos_read_register(CMOS_REG_HOURS);
		rtc.weekdays = cmos_read_register(CMOS_REG_WEEKDAYS);
		rtc.day = cmos_read_register(CMOS_REG_DAY);
		rtc.month = cmos_read_register(CMOS_REG_MONTH);
		rtc.year = cmos_read_register(CMOS_REG_YEAR);
		rtc.century = cmos_read_register(CMOS_REG_CENTURY);
	} while (rtc_values_are_not_equal(rtc, last));

	// Status Register B contains the formats of bytes
	u8 reg_b = cmos_read_register(CMOS_REG_STATUS_B);

	if (!(reg_b & 0x04)) { // Are we in BCD mode?
		// If so, convert it to "good" binary values
		rtc.seconds = (rtc.seconds & 0x0F) + ((rtc.seconds / 16) * 10);
		rtc.minutes = (rtc.minutes & 0x0F) + ((rtc.minutes / 16) * 10);
		rtc.hours = ((rtc.hours & 0x0F) + (((rtc.hours & 0x70) / 16 * 10))) | (rtc.hours & 0x80);
		rtc.weekdays = (rtc.weekdays & 0x0F) + ((rtc.weekdays / 16) * 10);
		rtc.month = (rtc.month & 0x0F) + ((rtc.month / 16) * 10);
		rtc.year = (rtc.year & 0x0F) + ((rtc.year / 16) * 10);
		rtc.century = (rtc.century & 0x0F) + ((rtc.century / 16) * 10);
	}

	// If the hour is pm, then the 0x80 bit is set on the hour byte
	if (!(reg_b & 0x02) && (rtc.hours & 0x80)) {
		rtc.hours = ((rtc.hours & 0x7F) + 12) % 24;
	}

	// Compute full year
	rtc.year += (rtc.century * 100);

	return rtc;
}

void cmos_rtc_handler(registers_state regs) {
	(void)regs; // get rid off 'unused' warning

	cmos_rtc_t date_and_time;

	date_and_time = cmos_read_rtc();	
	(void)date_and_time;

	// Read Status Register C so taht future IRQ8s can occur
	cmos_read_register(CMOS_REG_STATUS_C);
}

void cmos_rtc_init() {
	// Enable RTC
	u8 prev_reg_value = cmos_read_register(CMOS_REG_STATUS_B);
	port_outb(CMOS_COMMAND_PORT, 0x8B); // Select Register B and disable NMI, reading will 
										// reset to register D
	port_outb(CMOS_DATA_PORT, prev_reg_value | 0x40); // Set bit 6 to enable periodic interrupts
													  // at default rate of 1024 Hz
	cmos_read_register(CMOS_REG_STATUS_C); // Read status register C to clear out
										   // any pending IRQ8 interrupts

	// Register a handler for CMOS RTC
	register_interrupt_handler(IRQ8, cmos_rtc_handler);
	DEBUG("%s", "CMOS RTC has been initialized\r\n");
}

