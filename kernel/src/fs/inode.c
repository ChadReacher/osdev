#include <ext2.h>
#include <blk_dev.h>
#include <heap.h>
#include <string.h>
#include <panic.h>
#include <heap.h>

struct ext2_inode inodes_table[NR_INODE] = {{0,},};

void read_inode(struct ext2_inode *inode);
void write_inode(struct ext2_inode *inode);

void read_group_desc(struct ext2_blk_grp_desc *bgd, u32 group) {
	struct buffer *buf;
	u32 descs_per_block, group_desc_block, group_desc_idx, sz;

	if (!bgd) {
		panic("read_group_desc failed: bgd is NULL\r\n");
	}
	sz = sizeof(struct ext2_blk_grp_desc);
	descs_per_block = super_block.s_block_size / sz;
	group_desc_block = group / descs_per_block;
	group_desc_idx = group % descs_per_block;
	buf = read_blk(super_block.s_dev, super_block.s_first_data_block + 1 + group_desc_block);
	if (!buf) {
		panic("read_group_desc failed to read block group descriptor block "
			  "#%d\r\n", super_block.s_first_data_block + 1 + group_desc_block);
		return;
	}
	memcpy(bgd, (void *)(buf->b_data + (group_desc_idx * sz)), sz);
	free(buf->b_data);
	free(buf);
}

void write_group_desc(struct ext2_blk_grp_desc *bgd, u32 group) {
	struct buffer *buf;
	u32 descs_per_block, group_desc_block, group_desc_idx;

	descs_per_block = super_block.s_block_size / 
		(sizeof(struct ext2_blk_grp_desc));
	group_desc_block = group / descs_per_block;
	group_desc_idx = group % descs_per_block;
	buf = read_blk(super_block.s_dev, super_block.s_first_data_block + 1 + group_desc_block);
	if(!buf) {
		return;
	}
	memcpy((void *)(buf->b_data + (group_desc_idx * sizeof(struct ext2_blk_grp_desc))), 
			bgd, sizeof(struct ext2_blk_grp_desc));
	write_blk(buf);
	free(buf->b_data);
	free(buf);
}

static volatile i32 last_allocated_inode = 0;

struct ext2_inode *get_empty_inode() {
	struct ext2_inode *inode;
	i32 inr;

	inode = NULL;
	inr = last_allocated_inode;
	do {
		if (!inodes_table[inr].i_count) {
			inode = inodes_table + inr;
			break;
		}
		++inr;
		if (inr >= NR_INODE) {
			inr = 0;
		}
	} while (inr != last_allocated_inode);
	if (!inode) {
		panic("No free inodes");
	}
	last_allocated_inode = inr;
	if (inode->i_dirt) {
		write_inode(inode);
	}
	memset(inode, 0, sizeof(*inode));
	inode->i_count = 1;
	return inode;
}

struct ext2_inode *iget(u16 dev, u32 nr) {
	struct ext2_inode *inode;

	if (!dev) {
		panic("iget with dev 0\r\n");
	} else if (nr < 1) {
		panic("inode numbers should be >= 1\r\n");
	}
	inode = inodes_table;
	while (inode < inodes_table + NR_INODE) {
		if (inode->i_dev != dev || inode->i_num != nr) {
			++inode;
			continue;
		}
		++inode->i_count;
		return inode;
	}
	inode = get_empty_inode();
	inode->i_dev = dev;
	inode->i_num = nr;
	read_inode(inode);
	return inode;
}

void iput(struct ext2_inode *inode) {
	if (!inode) {
		return;
	}
	if (!inode->i_count) {
		debug("iput failed: inode #%d is already free\r\n", inode->i_num);
		return;
	}
	--inode->i_count;
	if (!inode->i_count) {
		if (!inode->i_links_count) {
			/* Free disk blocks, free inode in inode bitmap,  */
			/* free inode in inode array `inodes_table` */
			inode->i_size = 0;
			ext2_truncate(inode);
			free_inode(inode);
			return;
		}
		if (inode->i_dirt) {
			write_inode(inode);
		}
	}
}

void read_inode(struct ext2_inode *inode) {
	struct buffer *buf;
	struct ext2_blk_grp_desc *bgd; 
	u32 group, itable_block, idx, inodes_per_block, block_offset,
		offset_in_block;

	if (!inode) {
		debug("Cannot read to inode, inode is NULL\r\n");
		return;
	}
	group = (inode->i_num - 1) / super_block.s_inodes_per_group;
	bgd = malloc(sizeof(struct ext2_blk_grp_desc));
	read_group_desc(bgd, group);
	if (!bgd) {
		debug("Failed to read group descriptor #%d\r\n", group);
		return;
	}
	itable_block = bgd->bg_inode_table;
	idx = (inode->i_num - 1) % super_block.s_inodes_per_group;
	inodes_per_block = super_block.s_block_size / super_block.s_inode_size;
	block_offset = idx / inodes_per_block;

	buf = read_blk(inode->i_dev, itable_block + block_offset);
	if (!buf) {
		debug("Failed to read inode's block #%d\r\n", 
				itable_block + block_offset);
		free(bgd);
		return;
	}
	offset_in_block = idx % inodes_per_block;
	memcpy(inode, (void *)(buf->b_data + offset_in_block * super_block.s_inode_size),
			super_block.s_inode_size);
	free(bgd);
	free(buf->b_data);
	free(buf);
}

void write_inode(struct ext2_inode *inode) {
	struct buffer *buf;
	struct ext2_blk_grp_desc *bgd; 
	u32 group, itable_block, idx, inodes_per_block, block_offset,
		offset_in_block;

	if (!inode) {
		debug("Cannot write to inode, inode is NULL\r\n");
		return;
	}
	group = (inode->i_num - 1) / super_block.s_inodes_per_group;
	bgd = malloc(sizeof(struct ext2_blk_grp_desc));
	read_group_desc(bgd, group);
	if (!bgd) {
		debug("Failed to read group descriptor #%d\r\n", group);
		return;
	}
	itable_block = bgd->bg_inode_table;
	idx = (inode->i_num - 1) % super_block.s_inodes_per_group;
	inodes_per_block = super_block.s_block_size / super_block.s_inode_size;
	block_offset = idx / inodes_per_block;

	buf = read_blk(inode->i_dev, itable_block + block_offset);
	if (!buf) {
		debug("Failed to read inode's block #%d\r\n", 
			itable_block + block_offset);
		free(bgd);
		return;
	}
	offset_in_block = idx % inodes_per_block;
	memcpy((void *)(buf->b_data + offset_in_block * super_block.s_inode_size), inode,
			super_block.s_inode_size);
	write_blk(buf);
	if (!buf) {
		debug("Failed to write inode's block #%d\r\n", 
				itable_block + block_offset);
		free(bgd);
	}

	inode->i_dirt = 0;
	free(bgd);
	free(buf->b_data);
	free(buf);
}
