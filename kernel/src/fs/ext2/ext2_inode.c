#include <ext2.h>
#include <blk_dev.h>
#include <heap.h>
#include <string.h>
#include <panic.h>
#include <vfs.h>

extern struct file_ops ext2_file_ops;

struct vfs_inode_ops ext2_inode_file_ops = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ext2_truncate,
	NULL,
	NULL
};
struct vfs_inode_ops ext2_inode_dir_ops = {
	ext2_unlink,
	ext2_link,
	ext2_rmdir,
	ext2_mkdir,
	ext2_rename,
	NULL,
	ext2_lookup,
	ext2_create,
};
struct vfs_inode_ops ext2_inode_chr_ops = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};
struct vfs_inode_ops ext2_inode_blk_ops = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};
struct vfs_inode_ops ext2_inode_fifo_ops = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

i32 read_group_desc(struct ext2_blk_grp_desc *bgd, u32 group, struct vfs_superblock *vsb) {
	struct buffer *buf;
	u32 descs_per_block, group_desc_block, group_desc_idx, sz;

	if (!bgd) {
		debug("read_group_desc failed: bgd is NULL\r\n");
		return -1;
	}
	sz = sizeof(struct ext2_blk_grp_desc);
	descs_per_block = vsb->s_block_size / sz;
	group_desc_block = group / descs_per_block;
	group_desc_idx = group % descs_per_block;
	buf = read_blk(vsb->s_dev, vsb->u.ext2_sb.s_first_data_block + 1 + group_desc_block);
	if (!buf) {
		debug("read_group_desc failed to read block group descriptor block "
			  "#%d\r\n", vsb->u.ext2_sb.s_first_data_block + 1 + group_desc_block);
		return -1;
	}
	memcpy(bgd, (void *)(buf->b_data + (group_desc_idx * sz)), sz);
	free(buf->b_data);
	free(buf);
	return 0;
}

i32 write_group_desc(struct ext2_blk_grp_desc *bgd, u32 group, struct vfs_superblock *vsb) {
	struct buffer *buf;
	u32 descs_per_block, group_desc_block, group_desc_idx;

	descs_per_block = vsb->s_block_size / 
		(sizeof(struct ext2_blk_grp_desc));
	group_desc_block = group / descs_per_block;
	group_desc_idx = group % descs_per_block;
	buf = read_blk(vsb->s_dev, vsb->u.ext2_sb.s_first_data_block + 1 + group_desc_block);
	if(!buf) {
		return -1;
	}
	memcpy((void *)(buf->b_data + (group_desc_idx * sizeof(struct ext2_blk_grp_desc))), 
			bgd, sizeof(struct ext2_blk_grp_desc));
	write_blk(buf);
	free(buf->b_data);
	free(buf);
	return 0;
}

void ext2_read_inode(struct vfs_inode *vnode) {
	struct buffer *buf;
	struct ext2_blk_grp_desc bgd;
    struct ext2_inode raw_inode;
	u32 group, itable_block, idx, inodes_per_block, block_offset,
		offset_in_block;

	if (!vnode) {
		return;
	}
	group = (vnode->i_num - 1) / vnode->i_sb->u.ext2_sb.s_inodes_per_group;
	if (read_group_desc(&bgd, group, vnode->i_sb) != 0) {
		debug("Failed to read group descriptor #%d\r\n", group);
		return;
	}
	itable_block = bgd.bg_inode_table;
	idx = (vnode->i_num - 1) % vnode->i_sb->u.ext2_sb.s_inodes_per_group;
	inodes_per_block = vnode->i_sb->s_block_size / vnode->i_sb->u.ext2_sb.s_inode_size;
	block_offset = idx / inodes_per_block;

	buf = read_blk(vnode->i_dev, itable_block + block_offset);
	if (!buf) {
		debug("Failed to read inode's block #%d\r\n", 
				itable_block + block_offset);
		return;
	}
	offset_in_block = idx % inodes_per_block;
	memcpy(&raw_inode, (void *)(buf->b_data + offset_in_block * vnode->i_sb->u.ext2_sb.s_inode_size),
			vnode->i_sb->u.ext2_sb.s_inode_size);
	free(buf->b_data);
	free(buf);
    
	vnode->i_mode = raw_inode.i_mode;
	vnode->i_uid = raw_inode.i_uid;
	vnode->i_gid = raw_inode.i_gid;
	vnode->i_size = raw_inode.i_size;
	vnode->i_atime = raw_inode.i_atime;
	vnode->i_ctime = raw_inode.i_ctime;
	vnode->i_mtime = raw_inode.i_mtime;
	vnode->i_links_count = raw_inode.i_links_count;
	vnode->i_blocks = raw_inode.i_blocks;
	vnode->i_flags = raw_inode.i_flags;
    vnode->u.i_ext2.i_osd1 = raw_inode.i_osd1;
    vnode->u.i_ext2.i_generation = raw_inode.i_generation;
	vnode->u.i_ext2.i_file_acl = raw_inode.i_file_acl;
	vnode->u.i_ext2.i_dir_acl = raw_inode.i_dir_acl;
	vnode->u.i_ext2.i_faddr = raw_inode.i_faddr;

	for (i32 i = 0; i < EXT2_N_BLOCKS; ++i) {
		vnode->u.i_ext2.i_block[i] = raw_inode.i_block[i];
	}
	for (i32 i = 0; i < 12; ++i) {
        vnode->u.i_ext2.osd2[i] = raw_inode.osd2[i];
	}

    // TODO: next step?
	if (EXT2_S_ISREG(vnode->i_mode)) {
		vnode->i_ops = &ext2_inode_file_ops;
    } else if (EXT2_S_ISDIR(vnode->i_mode)) {
		vnode->i_ops = &ext2_inode_dir_ops;
    } else if (EXT2_S_ISCHR(vnode->i_mode)) {
		//vnode->i_ops = ext2_inode__chrdev_ops;
    } else if (EXT2_S_ISBLK(vnode->i_mode)) {
		//vnode->i_ops = ext2_inode__blkdev_ops;
    } else if (EXT2_S_ISFIFO(vnode->i_mode)) {
		//vnode->i_ops = ext2_inode__pipe_ops;
	}

	vnode->i_f_ops = &ext2_file_ops;
}

void ext2_write_inode(struct vfs_inode *vnode) {
	struct buffer *buf;
    struct ext2_inode raw_inode;
	struct ext2_blk_grp_desc bgd;
	u32 group, itable_block, idx, inodes_per_block, block_offset,
		offset_in_block;

	if (!vnode) {
		return;
	}
	group = (vnode->i_num - 1) / vnode->i_sb->u.ext2_sb.s_inodes_per_group;
	if (read_group_desc(&bgd, group, vnode->i_sb) != 0) {
		debug("Failed to read group descriptor #%d\r\n", group);
		return;
	}
	itable_block = bgd.bg_inode_table;
	idx = (vnode->i_num - 1) % vnode->i_sb->u.ext2_sb.s_inodes_per_group;
	inodes_per_block = vnode->i_sb->s_block_size / vnode->i_sb->u.ext2_sb.s_inode_size;
	block_offset = idx / inodes_per_block;

	raw_inode.i_mode = vnode->i_mode;
	raw_inode.i_uid = vnode->i_uid;
	raw_inode.i_size = vnode->i_size;
	raw_inode.i_atime = vnode->i_atime;
	raw_inode.i_ctime = vnode->i_ctime;
	raw_inode.i_mtime = vnode->i_mtime;
	raw_inode.i_dtime = vnode->u.i_ext2.i_dtime;
	raw_inode.i_gid = vnode->i_gid;
	raw_inode.i_links_count = vnode->i_links_count;
	raw_inode.i_blocks = vnode->i_blocks;
	raw_inode.i_flags = vnode->i_flags;
	raw_inode.i_osd1 = vnode->u.i_ext2.i_osd1;
	raw_inode.i_generation = vnode->u.i_ext2.i_generation;
	raw_inode.i_file_acl = vnode->u.i_ext2.i_file_acl;
	raw_inode.i_dir_acl = vnode->u.i_ext2.i_dir_acl;
	raw_inode.i_faddr = vnode->u.i_ext2.i_faddr;

	for (i32 i = 0; i < EXT2_N_BLOCKS; ++i) {
		raw_inode.i_block[i] = vnode->u.i_ext2.i_block[i];
	}
	for (i32 i = 0; i < 12; ++i) {
		raw_inode.osd2[i] = vnode->u.i_ext2.osd2[i];
	}

	buf = read_blk(vnode->i_dev, itable_block + block_offset);
	if (!buf) {
		debug("Failed to read inode's block #%d\r\n", 
			itable_block + block_offset);
		return;
	}
	offset_in_block = idx % inodes_per_block;
	memcpy((void *)(buf->b_data + offset_in_block * vnode->i_sb->u.ext2_sb.s_inode_size), &raw_inode,
			vnode->i_sb->u.ext2_sb.s_inode_size);
	write_blk(buf);
	if (!buf) {
		debug("Failed to write inode's block #%d\r\n", 
				itable_block + block_offset);
		return;
	}

	vnode->i_dirt = 0;
	free(buf->b_data);
	free(buf);
}
