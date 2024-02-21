#include <blk_dev.h>
#include <ata.h>

i32 block_write(u16 dev, u32 *pos, i8 *buf, u32 count) {
	u32 block = *pos / BLOCK_SIZE;
	u32 offset = *pos % BLOCK_SIZE;
	u32 chars;
	i8 *ret_buf, *p;
	u32 written = 0;

	while (count > 0) {
		chars = BLOCK_SIZE - offset;
		if (chars > count) {
			chars = count;
		}
		rw_block(READ, dev, block, &ret_buf);
		p = ret_buf + offset;
		memcpy(p, buf, chars);
		rw_block(WRITE, dev, block, &ret_buf);
		offset = 0;
		++block;
		*pos += chars;
		written += chars;
		count -= chars;
		buf += chars;
		free(ret_buf);
	}
	return written;
}

i32 block_read(u16 dev, u32 *pos, i8 *buf, u32 count) {
	u32 block = *pos / BLOCK_SIZE;
	u32 offset = *pos % BLOCK_SIZE;
	u32 chars;
	i8 *ret_buf, *p;
	u32 read = 0;

	while (count > 0) {
		chars = BLOCK_SIZE - offset;
		if (chars > count) {
			chars = count;
		}
		rw_block(READ, dev, block, &ret_buf);
		p = ret_buf + offset;
		offset = 0;
		++block;
		*pos += chars;
		read += chars;
		count -= chars;
		memcpy(buf, p, chars);
		buf += read;
		free(ret_buf);
	}
	return read;
}

typedef void (*blk_fn)(u32 rw, u16 dev, u32 block, i8 **buf);

static blk_fn blk_dev[] = {
	NULL,     // no dev
	NULL,     // dev mem
	NULL,     // dev fd
	rw_ata,   // dev hd
	NULL,     // dev ttyx
	NULL,     // dev tty
	NULL,     // dev lp
};

void rw_block(u32 rw, u16 dev, u32 block, i8 **buf) {
	blk_fn blk_addr;
	u16 major;
	if ((major=MAJOR(dev)) >= NRBLKDEV || !(blk_addr=blk_dev[major])) {
		kernel_panic("nonexistent block device");
	}
	blk_addr(rw, dev, block, buf);
}

