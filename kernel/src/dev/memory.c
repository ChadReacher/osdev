#include <memory.h>
#include <chr_dev.h>
#include <errno.h>

#define NR_MEM_DEVICES 6

static i32 dev_null_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
static i32 dev_null_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
static i32 dev_zero_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
static i32 dev_zero_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
static i32 dev_full_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
static i32 dev_full_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);

typedef i32 (*read_fn)(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
typedef i32 (*write_fn)(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
struct rw_ops {
    read_fn read;
    write_fn write;
};

static struct rw_ops rw_ops[NR_MEM_DEVICES] = {
    // /dev/mem
    { NULL, NULL },
    // /dev/kmem
    { NULL, NULL },
    // /dev/null
    { dev_null_read, dev_null_write },
    // /dev/port
    { NULL, NULL },
    // /dev/zero
    { dev_zero_read, dev_zero_write },
    // /dev/full
    { dev_full_read, dev_full_write },
};

struct file_ops dev_memory_ops = {
	dev_mem_open,
	dev_mem_read,
    dev_mem_write,
	NULL,
};

i32 dev_mem_open(UNUSED struct vfs_inode *inode, UNUSED struct file *fp) {
    return 0;
}

i32 dev_mem_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count) {
    u32 minor = MINOR(inode->i_rdev);

    if (minor > NR_MEM_DEVICES) {
        return -EINVAL;
    }
    return rw_ops[minor].read(inode, fp, buf, count);
}

i32 dev_mem_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count) {
    u32 minor = MINOR(inode->i_rdev);

    if (minor > NR_MEM_DEVICES) {
        return -EINVAL;
    }
    return rw_ops[minor].write(inode, fp, buf, count);
}

static i32 dev_null_read(UNUSED struct vfs_inode *inode, UNUSED struct file *fp,
                         UNUSED i8 *buf, UNUSED i32 count) {
    return 0;
}

static i32 dev_null_write(UNUSED struct vfs_inode *inode,
                          UNUSED struct file *fp, UNUSED i8 *buf, i32 count) {
    return count;
}

static i32 dev_zero_read(UNUSED struct vfs_inode *inode, UNUSED struct file *fp,
                         i8 *buf, i32 count) {
    for (i32 i = 0; i < count; ++i) {
        buf[i] = 0;
    }
    return count;
}

static i32 dev_zero_write(UNUSED struct vfs_inode *inode,
                          UNUSED struct file *fp, UNUSED i8 *buf, i32 count) {
    return count;
}

static i32 dev_full_read(UNUSED struct vfs_inode *inode, UNUSED struct file *fp,
                         i8 *buf, i32 count) {
    for (i32 i = 0; i < count; ++i) {
        buf[i] = 0;
    }
    return count;
}

static i32 dev_full_write(UNUSED struct vfs_inode *inode,
                          UNUSED struct file *fp, UNUSED i8 *buf,
                          UNUSED i32 count) {
    return -ENOSPC;
}
