#include <ext2.h>
#include <vfs.h>
#include <blk_dev.h>

extern struct file_ops *blk_dev_ops[];

struct file_ops ext2_blk_ops = {
	ext2_blk_open,
	NULL,
    NULL,
	NULL,
};

i32 ext2_blk_open(struct vfs_inode *inode, struct file *fp) {
	i32 i;

	i = MAJOR(inode->i_rdev);
	// TODO: move 7 to define
	if (i > 7) {
		return 0;
	}
	fp->f_ops = blk_dev_ops[i];
	if (fp->f_ops && fp->f_ops->open) {
		return fp->f_ops->open(inode, fp);
	}
	return 0;
}
