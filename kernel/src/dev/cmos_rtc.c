#include <cmos_rtc.h>
#include <string.h>
#include <port.h>
#include <panic.h>
#include <isr.h>

extern u32 startup_time;

#include "isr.h"

static bool rtc_values_are_not_equal(struct cmos_time left, struct cmos_time right) {
    return (
        left.second != right.second ||
        left.minute != right.minute ||
        left.hour != right.hour ||
        left.weekday != right.weekday ||
        left.day != right.day ||
        left.month != right.month ||
        left.year != right.year ||
        left.century != right.century
    );
}

static u8 cmos_read_register(u8 reg) {
    /*
     * To read a register, first you need to select it
     * Also you should disable NMI by setting 0x80 bit
     * After that you can read the value of the register
     * on CMOS_DATA_PORT(0x71)
     */
    port_outb(CMOS_COMMAND_PORT, (1 << 7 | reg));
    return port_inb(CMOS_DATA_PORT);
}

static bool cmos_update_in_progress(void) {
    port_outb(CMOS_COMMAND_PORT, CMOS_REG_STATUS_A);
    /* The RTC has an "Update in progress" flag (bit 7 of Status Register A) */
    return port_inb(CMOS_DATA_PORT) & 0x80;
}

static struct cmos_time cmos_read_rtc(void) {
    /*
     * First, you have to ensure that you won't be effected
     * by an update. Then select the associated "CMOS register"
     * and read value
     */
    u8 reg_b;
    struct cmos_time rtc;
    struct cmos_time last;

    /*
     * This uses the "read registers until you get the same values twice in a
     * row" technique to avoid getting inconsistent values due to RTC updates
     */
    while (cmos_update_in_progress());

    /* Read first time */
    rtc.second = cmos_read_register(CMOS_REG_SECONDS);
    rtc.minute = cmos_read_register(CMOS_REG_MINUTES);
    rtc.hour = cmos_read_register(CMOS_REG_HOURS);
    rtc.weekday = cmos_read_register(CMOS_REG_WEEKDAYS);
    rtc.day = cmos_read_register(CMOS_REG_DAY);
    rtc.month = cmos_read_register(CMOS_REG_MONTH);
    rtc.year = cmos_read_register(CMOS_REG_YEAR);
    rtc.century = cmos_read_register(CMOS_REG_CENTURY);

    do {
        /* Prepare to read for a second time */
        memcpy(&last, &rtc, sizeof(struct cmos_time));

        while (cmos_update_in_progress());

        /* Read second time */
        rtc.second = cmos_read_register(CMOS_REG_SECONDS);
        rtc.minute = cmos_read_register(CMOS_REG_MINUTES);
        rtc.hour = cmos_read_register(CMOS_REG_HOURS);
        rtc.weekday = cmos_read_register(CMOS_REG_WEEKDAYS);
        rtc.day = cmos_read_register(CMOS_REG_DAY);
        rtc.month = cmos_read_register(CMOS_REG_MONTH);
        rtc.year = cmos_read_register(CMOS_REG_YEAR);
        rtc.century = cmos_read_register(CMOS_REG_CENTURY);
    } while (rtc_values_are_not_equal(rtc, last));

    /* Status Register B contains the formats of bytes */
    reg_b = cmos_read_register(CMOS_REG_STATUS_B);

    /* Are we in BCD mode? */
    if (!(reg_b & 0x04)) {
        /* If so, convert it to "good" binary values */
        rtc.second      =   (rtc.second & 0x0F)  + ((rtc.second / 16) * 10);
        rtc.minute      =   (rtc.minute & 0x0F)  + ((rtc.minute / 16) * 10);
        rtc.hour        =   (rtc.hour & 0x0F)    + ((rtc.hour / 16) * 10);
        rtc.weekday     =   (rtc.weekday & 0x0F) + ((rtc.weekday / 16) * 10);
        rtc.day         =   (rtc.day & 0x0F)     + ((rtc.day / 16) * 10);
        rtc.month       =   (rtc.month & 0x0F)   + ((rtc.month / 16) * 10);
        rtc.year        =   (rtc.year & 0x0F)    + ((rtc.year / 16) * 10);
        rtc.century     =   (rtc.century & 0x0F) + ((rtc.century / 16) * 10);
    }

    /* If the hour is pm, then the 0x80 bit is set on the hour byte */
    if (!(reg_b & 0x02) && (rtc.hour & 0x80)) {
        rtc.hour = ((rtc.hour & 0x7F) + 12) % 24;
    }

    /* Compute full year */
    rtc.year += (rtc.century * 100);

    return rtc;
}

static void UNUSED cmos_rtc_handler(struct registers_state *regs) {
    (void)regs;

    cmos_read_rtc();

    /* Read Status Register C so that future IRQ8s can occur */
    cmos_read_register(CMOS_REG_STATUS_C);
}

#define MINUTE 60
#define HOUR (60*MINUTE)
#define DAY (24*HOUR)

static i8 month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

#define LEAP_YEAR(y) (((y % 4) == 0 && (y % 100) != 0) || (y % 400) == 0)
#define DAYS_PER_YEAR(y) ((LEAP_YEAR(y)) ? 366 : 365)

void setup_startup_time(void) {
    i32 i;
    struct cmos_time time;
    u32 total_days = 0, seconds;

    time = cmos_read_rtc();
    for (i = 1970; i < time.year; ++i) {
        total_days += DAYS_PER_YEAR(i);
    }
    for (i = 0; i < (time.month - 1); ++i) {
        total_days += month[i];
        if (i == 1) {
            total_days += LEAP_YEAR(time.year) ? 1 : 0;
        }
    }
    total_days += (time.day - 1);
    seconds = total_days * DAY;
    seconds += time.hour * HOUR;
    seconds += time.minute * MINUTE;
    seconds += time.second;
    startup_time = seconds;
}


void cmos_rtc_init(void) {
    /* Enable RTC */
    u8 prev_reg_value = cmos_read_register(CMOS_REG_STATUS_B);
    /* Select Register B and disable NMI, reading will reset to register D */
    port_outb(CMOS_COMMAND_PORT, 0x8B);
    /* Set bit 6 to enable periodic interrupts at default rate of 1024 Hz */
    port_outb(CMOS_DATA_PORT, prev_reg_value | 0x40);
    /* Read status register C to clear out any pending IRQ8 interrupts */
    cmos_read_register(CMOS_REG_STATUS_C);

    setup_startup_time();

    debug("CMOS RTC has been initialized\r\n");
}

