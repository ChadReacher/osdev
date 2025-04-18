#include <vfs.h>
#include <panic.h>
#include <string.h>
#include <heap.h>


static struct vfs_inode inodes_table[NR_INODES] = {{0,},};
static volatile i32 last_allocated_inode = 0;

void sync_inodes(void) {
	struct vfs_inode *inode = NULL;

    for (i32 i = 0; i < NR_INODES; ++i) {
		inode = &inodes_table[i];
		debug("i_flags - 0x%x, i_num - %d, i_count - %d, i_dirt - 0x%x, i_pipe - 0x%x\r\n",
			inode->i_flags, inode->i_num, inode->i_count, inode->i_dirt, inode->i_pipe);
                if (inode->i_dirt) {
                        if (inode->i_sb != NULL && inode->i_sb->fs_ops && inode->i_sb->fs_ops->write_inode) {
                                inode->i_sb->fs_ops->write_inode(inode);
                        }
                }
	}
}


/* get_empty_inode - gets a free, empty inode from inode cache */
struct vfs_inode *get_empty_inode() {
	struct vfs_inode *inode;
	i32 inr;

	inode = NULL;
	inr = last_allocated_inode;
	do {
		if (!inodes_table[inr].i_count) {
			inode = inodes_table + inr;
			break;
		}
		++inr;
		if (inr >= NR_INODES) {
			inr = 0;
		}
	} while (inr != last_allocated_inode);
	if (!inode) {
		panic("No free inodes");
	}
	last_allocated_inode = inr;
	if (inode->i_dirt) {
		if (inode->i_sb != NULL && inode->i_sb->fs_ops && inode->i_sb->fs_ops->write_inode) {
			inode->i_sb->fs_ops->write_inode(inode);
		}
	}
	memset(inode, 0, sizeof(*inode));
	inode->i_count = 1;
	return inode;
}

/* vfs_get - gets the VFS inode by its number and device id where it resides */
struct vfs_inode *vfs_iget(u16 dev, u32 nr) {
	struct vfs_inode *inode;

        if (!dev) {
		panic("vfs_iget with dev 0\r\n");
	} else if (nr < 1) {
		panic("inode numbers should be >= 1\r\n");
        }

        // Search for inode in inode cache
        for (i32 i = 0; i < NR_INODES; ++i) {
	        inode = &inodes_table[i];
                if (inode->i_dev != dev || inode->i_num != nr) {
                        continue;
		}
		++inode->i_count;
		debug("[icache]: got the cached inode - %d, now count - %d\r\n", nr, inode->i_count);
		return inode;
	}

	inode = get_empty_inode();
	debug("[icache]: got the empty inode from cache - %d\r\n", nr);
	inode->i_dev = dev;
	inode->i_num = nr;
        inode->i_sb = get_vfs_super(dev);
        if (!inode->i_sb) {
                vfs_iput(inode);
                return NULL;
        }
        if (inode->i_sb->fs_ops && inode->i_sb->fs_ops->read_inode) {
                inode->i_sb->fs_ops->read_inode(inode);
        }
	return inode;
}

/* vfs_iput - release a VFS inode */
void vfs_iput(struct vfs_inode *inode) {
	if (!inode) {
		return;
	}
	if (!inode->i_count) {
		debug("vfs_iput failed: inode #%d is already free\r\n", inode->i_num);
		return;
	}
	if (inode->i_pipe) {
		process_wakeup(&inode->i_wait);
		if (--inode->i_count) {
			debug("[icache]: decreased the 'i_count', now - %d for the inode - %d\r\n", inode->i_count, inode->i_num);
			return;
		}
		debug("[icache]: freeing the pipe inode - %d\r\n", inode->i_num);
		free((void *)inode->u.i_pipe.i_buf);
		inode->i_count = 0;
		inode->i_dirt = 0;
		inode->i_pipe = 0;
		inode->u.i_pipe.i_head = inode->u.i_pipe.i_tail = 0;
		return;
	}
	--inode->i_count;
	debug("[icache]: decreased the 'i_count', now - %d for the inode - %d\r\n", inode->i_count, inode->i_num);
	if (!inode->i_count) {
		debug("[icache]: freeing the inode - %d\r\n", inode->i_num);
		if (!inode->i_links_count) {
			/* Free disk blocks, free inode in inode bitmap, */
			/* free inode in inode array `inodes_table` */
			inode->i_size = 0;
                        if (inode->i_ops && inode->i_ops->truncate) {
                                inode->i_ops->truncate(inode, 0);
                        }
                        if (inode->i_sb && inode->i_sb->fs_ops && inode->i_sb->fs_ops->free_inode) {
                                inode->i_sb->fs_ops->free_inode(inode);
                        }
			return;
		}
		if (inode->i_dirt) {
                        if (inode->i_sb && inode->i_sb->fs_ops && inode->i_sb->fs_ops->write_inode) {
                                debug("inode is dirt, writing to the disk\r\n");
                                inode->i_sb->fs_ops->write_inode(inode);
                        }
		}
	}
}
