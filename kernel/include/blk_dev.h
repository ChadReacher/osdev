#ifndef BLK_DEV
#define BLK_DEV

#include <types.h>
#include <vfs.h>

#define READ 0
#define WRITE 1

#define NRBLKDEV 7

#define BLOCK_SIZE 1024

#define MAJOR(a) ((unsigned)(a)>>8)
#define MINOR(a) ((a)&0xFF)

i32 block_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
i32 block_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);

void blk_dev_read(struct buffer *buf);
void blk_dev_write(struct buffer *buf);

#endif
