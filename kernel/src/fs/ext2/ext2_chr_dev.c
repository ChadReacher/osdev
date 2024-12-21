#include <ext2.h>
#include <vfs.h>
#include <chr_dev.h>

extern struct file_ops *chr_dev_ops[];

struct file_ops ext2_chr_ops = {
	ext2_chr_open,
	NULL,
    NULL,
	NULL,
};

i32 ext2_chr_open(struct vfs_inode *inode, struct file *fp) {
	i32 i;

	i = MAJOR(inode->i_rdev);
	if (i > NRCHRDEV) {
		return 0;
	}
	fp->f_ops = chr_dev_ops[i];
	if (fp->f_ops && fp->f_ops->open) {
		return fp->f_ops->open(inode, fp);
	}
	return 0;
}
