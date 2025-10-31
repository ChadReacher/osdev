#ifndef MEMORY_H
#define MEMORY_H

#include <types.h>
#include <panic.h>
#include <vfs.h>

extern struct file_ops dev_memory_ops;

i32 dev_mem_open(struct vfs_inode *inode, struct file *fp);
i32 dev_mem_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
i32 dev_mem_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);

#endif
