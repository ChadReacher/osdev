#include <ext2.h>
#include <panic.h>
#include <process.h>
#include <string.h>
#include <errno.h>


static struct filesystem filesystems[NR_FILESYSTEMS] = {
    { "ext2", ext2_read_super },
};

// Super blocks are mounted file systems
struct vfs_superblock superblocks[NR_SUPERBLOCKS];


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
        vsb->s_dev = 0;
        return NULL;
    }
    return vsb;
    
}

i32 vfs_do_mount(u32 dev, struct vfs_inode *dir) {
    for (i32 i = 0; i < NR_FILESYSTEMS; ++i) {
        struct vfs_superblock *vsb = read_super(dev, &filesystems[i]);
        if (!vsb) {
            continue;
        }
        vsb->s_mounted = dir;
        // increment the inode count because it is used for a mount
        ++vsb->s_mounted->i_count;
        // We don't really use it until lookup
        // but keep reference to substitute during inode lookup
        vsb->s_root->i_count = 1;
        return 0;
    }
    return -ENODEV;
}

void mount_root(void) {
    for (i32 i = 0; i < NR_FILESYSTEMS; ++i) {
        struct vfs_superblock *vsb = read_super(ROOT_DEV, &filesystems[i]);
        if (vsb) {
            // We logically use the root inode 2 times:
            // as a root directory and as a current directory for the init process.
            // 'vsb->s_root' is a temporary field to pass the root inode.
            vsb->s_root->i_count += 1;
            vsb->s_mounted = NULL;
            current_process->root = vsb->s_root;
            current_process->pwd = vsb->s_root;
            current_process->str_pwd = strdup("/");
            debug("The %s filesystem has been mounted as root\r\n", filesystems[i].name);
            return;
        }
    }
    panic("Could not mount the root\r\n");
}

