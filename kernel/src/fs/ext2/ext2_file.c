#include <ext2.h>
#include <blk_dev.h>
#include <panic.h>
#include <timer.h>
#include <errno.h>
#include <string.h>
#include <heap.h>
#include <fcntl.h>
#include <vfs.h>

struct file_ops ext2_file_ops = {
	NULL,
	ext2_file_read,
    ext2_file_write,
	ext2_readdir,
};

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

i32 ext2_file_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count) {
	struct buffer *lbuf;
	i32 left, block, inblock, chars;

	if ((left = count) <= 0) {
		return 0;
	}
	while (left) {
		block = ext2_bmap(inode, fp->f_pos);
		if (block) {
			lbuf = read_blk(inode->i_dev, block);
			if (!lbuf) {
				break;
			}
		}
		inblock = fp->f_pos % BLOCK_SIZE;
		chars = MIN(BLOCK_SIZE - inblock, left);
		fp->f_pos += chars;
		left -= chars;
		if (block) {
			i8 *p = lbuf->b_data + inblock;
			memcpy(buf, p, chars);
			free(lbuf->b_data);
			free(lbuf);
		} else {
			memset(buf, 0, chars);
		}
		buf += chars;
	}
	inode->i_atime = get_current_time();
	return (count-left)?(count-left):-ERROR;
}

i32 ext2_file_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count) {
	i8 *p;
	struct buffer *lbuf;
	i32 pos;
	u32 block, inblock, chars;
	u32 left = count;

	if (fp->f_flags & O_APPEND) {
		pos = inode->i_size;
	} else {
		pos = fp->f_pos;
	}
	while (left) {
		if (!(block = ext2_create_block(inode, pos))) {
			break;
		}
		lbuf = read_blk(inode->i_dev, block);
		if (!lbuf) {
			break;
		}
		inblock = pos % BLOCK_SIZE;
		chars = MIN(BLOCK_SIZE - inblock, left);
		pos += chars;
		if ((u32)pos > inode->i_size) {
			inode->i_size = pos;
			inode->i_dirt = 1;
		}
		left -= chars;
		
		p = lbuf->b_data + inblock;
		memcpy(p, buf, chars);
		write_blk(lbuf);
		free(lbuf->b_data);
		free(lbuf);

		buf += chars;
	}
	if (!(fp->f_flags & O_APPEND)) {
		fp->f_pos = pos;
		inode->i_ctime = get_current_time();
	}
	inode->i_mtime = get_current_time();
	return (count - left) ? (i32)(count - left) : -1;
}

i32 ext2_readdir(struct vfs_inode *inode, struct file *fp, 
		struct dirent *dent) {
	u32 block, offset, i;
	struct buffer *buf;
	struct ext2_dir *de;

	if (!inode || !EXT2_S_ISDIR(inode->i_mode)) {
		return -EBADF;
	}
	while ((u32)fp->f_pos < inode->i_size) {
		offset = fp->f_pos & 1023;
		block = ext2_bmap(inode, fp->f_pos);
		if (!block || !(buf = read_blk(inode->i_dev, block))) {
			fp->f_pos += 1024 - offset;
			continue;
		}
		de = (struct ext2_dir *)(buf->b_data + offset);
		while (offset < 1024 && (u32)fp->f_pos < inode->i_size) {
			offset += de->rec_len;
			fp->f_pos += de->rec_len;
			if (de->inode) {
				memcpy(dent->name, de->name, de->name_len);
				dent->inode = de->inode;
				dent->name[de->name_len] = '\0';
				i = de->name_len;
				free(buf->b_data);
				free(buf);
				return i;
			}
			de = (struct ext2_dir *)(buf->b_data + offset);
		}
	}
	return 0;
}
