#include <blk_dev.h>
#include <ata.h>
#include <heap.h>
#include <string.h>
#include <panic.h>
#include <vfs.h>

struct file_ops ata_ops = {
	NULL,
	block_read,
	block_write,
	NULL,
};

struct file_ops *blk_dev_ops[] = {
	NULL,     /* no dev */
	NULL,     /* dev mem */
	NULL,     /* dev fd */
	&ata_ops,	  /* dev hd */
	NULL,     /* dev ttyx */
	NULL,	  /* dev tty */
	NULL,     /* dev lp */
};

i32 block_write(struct vfs_inode *inode, struct file *fp, i8 *buf, u32 count) {
	u32 block = fp->f_pos / BLOCK_SIZE;
	u32 offset = fp->f_pos % BLOCK_SIZE;
	u32 chars;
	struct buffer *ret_buf;
	i8 *p;
	u32 written = 0;

	while (count > 0) {
		chars = BLOCK_SIZE - offset;
		if (chars > count) {
			chars = count;
		}
		ret_buf = read_blk(inode->i_rdev, block);
		if (ret_buf == NULL) {
			return written;
		}
		p = ret_buf->b_data + offset;
		memcpy(p, buf, chars);
		write_blk(ret_buf);
		offset = 0;
		++block;
		fp->f_pos += chars;
		written += chars;
		count -= chars;
		buf += chars;
		free(ret_buf);
		free(ret_buf->b_data);
	}
	return written;
}

i32 block_read(struct vfs_inode *inode, struct file *fp, i8 *buf, u32 count) {
	u32 block = fp->f_pos / BLOCK_SIZE;
	u32 offset = fp->f_pos % BLOCK_SIZE;
	u32 chars;
	struct buffer *ret_buf; 
	i8 *p;
	u32 read = 0;

	while (count > 0) {
		chars = BLOCK_SIZE - offset;
		if (chars > count) {
			chars = count;
		}
		ret_buf = read_blk(inode->i_rdev, block);
		if (ret_buf == NULL) {
			return read;
		}
		p = ret_buf->b_data + offset;
		offset = 0;
		++block;
		fp->f_pos += chars;
		read += chars;
		count -= chars;
		memcpy(buf, p, chars);
		buf += read;
		free(ret_buf->b_data);
		free(ret_buf);
	}
	return read;
}

typedef void (*blk_fn)(u32 rw, u16 dev, u32 block, i8 **buf);

static blk_fn blk_dev[] = {
	NULL,     /* no dev */
	NULL,     /* dev mem */
	NULL,     /* dev fd */
	rw_ata,   /* dev hd */
	NULL,     /* dev ttyx */
	NULL,     /* dev tty */
	NULL,     /* dev lp */
};


struct buffer *read_blk(u16 dev, u32 block) {
	struct buffer *buf;
	i8 *data;
	blk_fn blk_addr;
	u16 major;
	if ((major=MAJOR(dev)) >= NRBLKDEV || !(blk_addr=blk_dev[major])) {
		panic("nonexistent block device");
	}
	blk_addr(READ, dev, block, &data);
	if (!data) {
		return NULL;
	}
	buf = malloc(sizeof(struct buffer));
	buf->b_dev = dev;
	buf->b_block = block;
	buf->b_data = data;
	return buf;
}

void write_blk(struct buffer *buf) {
	blk_fn blk_addr;
	u16 major;

	if (!buf) {
		return;
	}
	if ((major=MAJOR(buf->b_dev)) >= NRBLKDEV || !(blk_addr=blk_dev[major])) {
		panic("nonexistent block device");
	}
	blk_addr(WRITE, buf->b_dev, buf->b_block, &buf->b_data);
}
