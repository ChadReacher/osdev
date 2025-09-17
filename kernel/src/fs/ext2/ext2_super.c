#include <ext2.h>
#include <heap.h>
#include <blk_dev.h>
#include <panic.h>
#include <process.h>
#include <string.h>
#include <bcache.h>
#include <timer.h>

struct fs_ops ext2_fs_ops = {
    ext2_read_inode,
    ext2_write_inode,
    ext2_free_inode
};

struct sb_ops ext2_sb_ops = {
    ext2_read_super,
    ext2_write_super
};

static void dump_super_block_info(struct ext2_super_block *sb);

i32 ext2_read_super(struct vfs_superblock *vsb) {
    struct vfs_inode *vnode;

    struct buffer *buf = bread(vsb->s_dev, 1);
    if (!buf) {
        debug("Failed to read block #1 on device %d\r\n");
        vsb->s_dev = 0;
        return -1;
    }
    memcpy(&vsb->u.ext2_sb, buf->data, sizeof(struct ext2_super_block));
    brelse(buf);
    if (vsb->u.ext2_sb.s_magic != EXT2_SUPER_MAGIC) {
        debug("It is not an EXT2 file system\r\n");
        vsb->s_dev = 0;
        return -1;
    }
    dump_super_block_info(&vsb->u.ext2_sb);
    vsb->s_block_size = 1024 << vsb->u.ext2_sb.s_log_block_size;
    vsb->fs_ops = &ext2_fs_ops;
    vsb->sb_ops = &ext2_sb_ops;
    if (!(vnode = vfs_iget(vsb->s_dev, EXT2_ROOT_INO))) {
        debug("Could not get the root inode");
        vsb->s_dev = 0;
        return -1;
    }
    vsb->s_root = vnode;

    ++vsb->u.ext2_sb.s_mnt_count;
    vsb->u.ext2_sb.s_mtime = get_current_time();

    return 0;
}

i32 ext2_write_super(struct vfs_superblock *vsb) {
    struct buffer *buf = bread(vsb->s_dev, 1);
    if (!buf) {
        debug("Failed to read block #1 on device %d\r\n");
        return -1;
    }

    memcpy(buf->data, &vsb->u.ext2_sb, sizeof(struct ext2_super_block));

    bwrite(buf);
    brelse(buf);

    return 0;
}

static void dump_super_block_info(struct ext2_super_block *sb) {
    debug("s_blocks_count      = %d\r\n", sb->s_blocks_count);
    debug("s_inodes_count      = %d\r\n", sb->s_inodes_count);
    debug("s_blocks_count      = %d\r\n", sb->s_blocks_count);
    debug("s_r_blocks_count    = %d\r\n", sb->s_r_blocks_count);
    debug("s_free_blocks_count = %d\r\n", sb->s_free_blocks_count);
    debug("s_free_inodes_count = %d\r\n", sb->s_free_inodes_count);
    debug("s_first_data_block  = %d\r\n", sb->s_first_data_block);
    debug("s_log_block_size    = %d\r\n", sb->s_log_block_size);
    debug("s_log_frag_size     = %d\r\n", sb->s_log_frag_size);
    debug("s_blocks_per_group  = %d\r\n", sb->s_blocks_per_group);
    debug("s_frags_per_group   = %d\r\n", sb->s_frags_per_group);
    debug("s_inodes_per_group  = %d\r\n", sb->s_inodes_per_group);
    debug("s_mtime             = %d\r\n", sb->s_mtime);
    debug("s_wtime             = %d\r\n", sb->s_wtime);
    debug("s_mnt_count         = %d\r\n", sb->s_mnt_count);
    debug("s_max_mnt_count     = %d\r\n", sb->s_max_mnt_count);
    debug("s_magic             = %#x\r\n", sb->s_magic);
    debug("s_state             = %d\r\n", sb->s_state);
    debug("s_errors            = %d\r\n", sb->s_errors);
    debug("s_minor_rev_level   = %d\r\n", sb->s_minor_rev_level);
    debug("s_lastcheck         = %d\r\n", sb->s_lastcheck);
    debug("s_checkinterval     = %d\r\n", sb->s_checkinterval);
    debug("s_creator_os        = %d\r\n", sb->s_creator_os);
    debug("s_rev_level         = %d\r\n", sb->s_rev_level);
    debug("s_def_resuid        = %d\r\n", sb->s_def_resuid);
    debug("s_def_resgid        = %d\r\n", sb->s_def_resgid);
}
