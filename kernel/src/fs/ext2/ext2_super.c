#include <ext2.h>
#include <heap.h>
#include <blk_dev.h>
#include <panic.h>
#include <process.h>
#include <string.h>

struct fs_ops ext2_fs_ops = {
    ext2_read_inode,
    ext2_write_inode,
    ext2_free_inode
};

static void dump_super_block_info(struct ext2_super_block *sb);

i32 ext2_read_super(struct vfs_superblock *vsb) {
	struct buffer *buf;
	struct vfs_inode *vnode;

	buf = read_blk(vsb->s_dev, 1);
	if (!buf) {
		debug("Failed to read block #1 on device %d\r\n");
		return -1;
	}
	vsb->u.ext2_sb = *((struct ext2_super_block *) buf->b_data);
	if (vsb->u.ext2_sb.s_magic != EXT2_SUPER_MAGIC) {
        vsb->s_dev = 0;
		debug("It is not an EXT2 file system\r\n");
		free(buf->b_data);
		free(buf);
		return -1;
	}
	dump_super_block_info(&vsb->u.ext2_sb);
	free(buf->b_data);
	free(buf);
    vsb->s_block_size = 1024 << vsb->u.ext2_sb.s_log_block_size;
    vsb->fs_ops = &ext2_fs_ops;
    if (!(vnode = vfs_iget(vsb->s_dev, EXT2_ROOT_INO))) {
        vsb->s_dev = 0;
        free(buf->b_data);
        free(buf);
        debug("Could not get the root inode");
        return -1;
    }
    vsb->s_root = vnode;
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
	debug("s_magic             = 0x%x\r\n", sb->s_magic);
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
