#include <ext2.h>
#include <heap.h>
#include <blk_dev.h>
#include <panic.h>
#include <process.h>
#include <string.h>


static struct filesystem filesystems[NR_FILESYSTEMS] = {
	{ "ext2", ext2_read_super },
	//{ "msdos", msdos_fs_ops },
};

// Super blocks are mounted file systems
static struct vfs_superblock superblocks[NR_SUPERBLOCKS];

struct vfs_superblock *get_vfs_super(u32 dev) {
    if (!dev) {
        return NULL;
    }
    for (int i = 0; i < NR_SUPERBLOCKS; ++i) {
        if (superblocks[i].s_dev == dev) {
            return &superblocks[i];
        }
    }
    return NULL;
}

static struct vfs_superblock *read_super(u32 dev, struct filesystem *fs) {
	struct vfs_superblock *vsb = NULL;

    for (i32 i = 0; i < NR_SUPERBLOCKS; ++i) {
        if (superblocks[i].s_dev == 0) {
            vsb = &superblocks[i];
            break;
        }
    }
    if (vsb == NULL) {
        return NULL;
    }

    vsb->s_dev = dev;
    if (fs->read_super(vsb) != 0) {
        return NULL;
    }
    return vsb;
    
}

void mount_root(void) {
    memset(superblocks, 0, sizeof(superblocks));

    for (i32 i = 0; i < NR_FILESYSTEMS; ++i) {
        struct vfs_superblock *vsb = read_super(ROOT_DEV, &filesystems[i]);
        if (vsb) {
			// We logically use the root inode 2 times: 
            // as a root directory and as current directory for init process. 
			// 'vsb->s_root' is a temporary field to pass the root inode.
            vsb->s_root->i_count += 1; 
	        current_process->root = vsb->s_root;
	        current_process->pwd = vsb->s_root;
	        current_process->str_pwd = strdup("/");
			return;
        }
    }
    panic("Could not mount the root");
}

