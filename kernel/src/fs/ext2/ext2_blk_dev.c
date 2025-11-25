#include <ext2.h>
#include <blk_dev.h>

extern struct file_ops *blk_dev_ops[];

struct file_ops ext2_blk_ops = {
    ext2_blk_open,
    NULL,
    NULL,
    NULL,
};

i32 ext2_blk_open(struct vfs_inode *inode, struct file *fp) {
    u32 major = MAJOR(inode->i_rdev);
    if (major > NRBLKDEV) {
        return -1;
    }
    fp->f_ops = blk_dev_ops[major];
    if (fp->f_ops && fp->f_ops->open) {
        return fp->f_ops->open(inode, fp);
    }
    return -1;
}
