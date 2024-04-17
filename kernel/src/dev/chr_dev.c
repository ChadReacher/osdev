#include "chr_dev.h"
#include "tty.h" 
#include "errno.h"
#include "process.h"

static i32 rw_ttyx(i32 rw, u16 minor, i8 *buf, i32 count) {
	return (rw == READ) ? tty_read(minor, buf, count) : tty_write(minor, buf, count);
}

static i32 rw_tty(i32 rw, u16 minor, i8 *buf, i32 count) {
	if (current_process->tty < 0) {
		return -EPERM;
	}
	return rw_ttyx(rw, minor, buf, count);
}

typedef i32 (*chr_fn)(i32 rw, u16 dev, i8 *buf, i32 count);

static chr_fn chr_dev[] = {
	NULL,     /* no dev */
	NULL,     /* dev mem */
	NULL,     /* dev fd */
	NULL,	  /* dev hd */
	rw_ttyx,  /* dev ttyx */
	rw_tty,	  /* dev tty */
	NULL,     /* dev lp */
};

i32 char_write(u16 dev, i8 *buf, u32 count) {
	chr_fn chr_addr;
	
	if (MAJOR(dev) > 7) {
		return -1;
	}
	if (!(chr_addr = chr_dev[MAJOR(dev)])) {
		return -1;
	}
	return chr_addr(WRITE, MINOR(dev), buf, count);
}

i32 char_read(u16 dev, i8 *buf, u32 count) {
	chr_fn chr_addr;
	
	if (MAJOR(dev) > 7) {
		return -1;
	}
	if (!(chr_addr = chr_dev[MAJOR(dev)])) {
		return -1;
	}
	return chr_addr(READ, MINOR(dev), buf, count);
}
