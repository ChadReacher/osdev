#include <ext2.h>
#include <heap.h>
#include <blk_dev.h>
#include <panic.h>
#include <process.h>
#include <string.h>

extern process_t *current_process;
struct ext2_super_block super_block;
static void dump_super_block_info();

static struct ext2_super_block *do_mount(u16 dev) {
	struct buffer *buf;
	struct ext2_super_block *p;

	p = &super_block;
	buf = read_blk(dev, 1);
	if (!buf) {
		debug("Failed to read block #1 on device %d\r\n");
		return NULL;
	}
	*p = *((struct ext2_super_block *) buf->b_data);
	if (p->s_magic != EXT2_SUPER_MAGIC) {
		debug("It is not an EXT2 file system\r\n");
		free(buf->b_data);
		free(buf);
		return NULL;
	}
	p->s_dev = dev;
	p->s_block_size = 1024 << p->s_log_block_size;
	p->s_total_groups = p->s_blocks_count / p->s_blocks_per_group;
	dump_super_block_info();
	free(buf->b_data);
	free(buf);
	return p;
}

void mount_root(void) {
	struct ext2_super_block *p;
	struct ext2_inode *inode;

	if (!(p = do_mount(ROOT_DEV))) {
		panic("Could not mount the root");
	}
	if (!(inode = iget(p->s_dev, 2))) {
		panic("Could not get the root inode");
	}
	current_process->root = inode;
	current_process->pwd = inode;
	current_process->str_pwd = strdup("/");
	inode->i_count = 2;
}

static void dump_super_block_info() {
	debug("s_blocks_count      = %d\r\n", super_block.s_blocks_count);
	debug("s_total_groups      = %d\r\n", super_block.s_total_groups);
	debug("s_inodes_count      = %d\r\n", super_block.s_inodes_count);
	debug("s_blocks_count      = %d\r\n", super_block.s_blocks_count);
	debug("s_r_blocks_count    = %d\r\n", super_block.s_r_blocks_count);
	debug("s_free_blocks_count = %d\r\n", super_block.s_free_blocks_count);
	debug("s_free_inodes_count = %d\r\n", super_block.s_free_inodes_count);
	debug("s_first_data_block  = %d\r\n", super_block.s_first_data_block);
	debug("s_log_block_size    = %d\r\n", super_block.s_log_block_size);
	debug("s_log_frag_size     = %d\r\n", super_block.s_log_frag_size);
	debug("s_blocks_per_group  = %d\r\n", super_block.s_blocks_per_group);
	debug("s_frags_per_group   = %d\r\n", super_block.s_frags_per_group);
	debug("s_inodes_per_group  = %d\r\n", super_block.s_inodes_per_group);
	debug("s_mtime             = %d\r\n", super_block.s_mtime);
	debug("s_wtime             = %d\r\n", super_block.s_wtime);
	debug("s_mnt_count         = %d\r\n", super_block.s_mnt_count);
	debug("s_max_mnt_count     = %d\r\n", super_block.s_max_mnt_count);
	debug("s_magic             = 0x%x\r\n", super_block.s_magic);
	debug("s_state             = %d\r\n", super_block.s_state);
	debug("s_errors            = %d\r\n", super_block.s_errors);
	debug("s_minor_rev_level   = %d\r\n", super_block.s_minor_rev_level);
	debug("s_lastcheck         = %d\r\n", super_block.s_lastcheck);
	debug("s_checkinterval     = %d\r\n", super_block.s_checkinterval);
	debug("s_creator_os        = %d\r\n", super_block.s_creator_os);
	debug("s_rev_level         = %d\r\n", super_block.s_rev_level);
	debug("s_def_resuid        = %d\r\n", super_block.s_def_resuid);
	debug("s_def_resgid        = %d\r\n", super_block.s_def_resgid);
}
