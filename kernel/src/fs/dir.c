#include <ext2.h> 
#include <syscall.h>
#include <errno.h>
#include <blk_dev.h>
#include <string.h>
#include <heap.h>

i32 ext2_readdir(struct ext2_inode *inode, struct file *fp, 
		struct dirent *dent) {
	u32 block, offset, i;
	struct buffer *buf;
	struct ext2_dir *de;

	if (!inode || !EXT2_S_ISDIR(inode->i_mode)) {
		return -EBADF;
	}
	while (fp->f_pos < inode->i_size) {
		offset = fp->f_pos & 1023;
		block = ext2_bmap(inode, fp->f_pos);
		if (!block || !(buf = read_blk(inode->i_dev, block))) {
			fp->f_pos += 1024 - offset;
			continue;
		}
		de = (struct ext2_dir *)(buf->b_data + offset);
		while (offset < 1024 && fp->f_pos < inode->i_size) {
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
