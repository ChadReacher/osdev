#include <ext2.h>
#include <blk_dev.h>
#include <timer.h>
#include <heap.h>

// TODO: For now we assume truncate to size 0
static void trunc_direct(struct ext2_inode *inode) {
	for (u32 i = 0; i < 12; ++i) {
		if (!inode->i_block[i]) {
			continue;
		}
		free_block(inode->i_dev, inode->i_block[i]);
		inode->i_block[i] = 0;
		inode->i_blocks -= 2;
	}
}

static void trunc_indirect(struct ext2_inode *inode) {
	struct buffer *buf;
	u32 *blocks;

	if (!inode->i_block[12]) {
		return;
	}
	buf = read_blk(inode->i_dev, inode->i_block[12]);
	//rw_block(READ, inode->i_dev, inode->i_block[12], &buf);
	blocks = (u32 *)buf->b_data;
	for (u32 i = 0; i < 256; ++i) {
		if (!blocks[i]) {
			continue;
		}
		free_block(inode->i_dev, blocks[i]);
		blocks[i] = 0;
		inode->i_blocks -= 2;
	}
	write_blk(buf);
	//rw_block(WRITE, inode->i_dev, inode->i_block[12], &buf);
	free_block(inode->i_dev, inode->i_block[12]);
	inode->i_block[12] = 0;
	inode->i_blocks -= 2;
	free(buf->b_data);
	free(buf);
}

static void trunc_doubly_indirect(struct ext2_inode *inode) {
	struct buffer *buf, *buf2;
	u32 *indirect, *blocks;

	if (!inode->i_block[13]) {
		return;
	}
	buf = read_blk(inode->i_dev, inode->i_block[13]);
	//rw_block(READ, inode->i_dev, inode->i_block[13], &buf);
	indirect = (u32 *)buf->b_data;
	for (u32 i = 0; i < 256; ++i) {
		if (!indirect[i]) {
			continue;
		}
		buf2 = read_blk(inode->i_dev, indirect[i]);
		//rw_block(READ, inode->i_dev, indirect[i], &buf2);
		blocks = (u32 *)buf2->b_data;
		for (u32 j = 0; j < 256; ++j) {
			if (!blocks[j]) {
				continue;
			}
			free_block(inode->i_dev, blocks[j]);
			blocks[j] = 0;
			inode->i_blocks -= 2;
		}
		write_blk(buf2);
		//rw_block(WRITE, inode->i_dev, indirect[i], &buf2);

		free_block(inode->i_dev, indirect[i]);
		indirect[i] = 0;
		inode->i_blocks -= 2;

		free(buf2->b_data);
		free(buf2);
	}
	write_blk(buf);
	//rw_block(WRITE, inode->i_dev, inode->i_block[13], &buf);
	free_block(inode->i_dev, inode->i_block[13]);
	inode->i_block[13] = 0;
	inode->i_blocks -= 2;
	free(buf->b_data);
	free(buf);
}

static void trunc_triply_indirect(struct ext2_inode *inode) {
	struct buffer *buf, *buf2, *buf3;
	u32 *dindirect, *indirect, *blocks;

	if (!inode->i_block[14]) {
		return;
	}
	buf = read_blk(inode->i_dev, inode->i_block[14]);
	//rw_block(READ, inode->i_dev, inode->i_block[14], &buf);
	dindirect = (u32 *)buf->b_data;
	for (u32 i = 0; i < 256; ++i) {
		if (!dindirect[i]) {
			continue;
		}
		buf2 = read_blk(inode->i_dev, dindirect[i]);
		//rw_block(READ, inode->i_dev, dindirect[i], &buf2);
		indirect = (u32 *)buf2->b_data;
		for (u32 j = 0; j < 256; ++j) {
			if (!indirect[j]) {
				continue;
			}
			buf3 = read_blk(inode->i_dev, indirect[j]);
			//rw_block(READ, inode->i_dev, indirect[j], &buf3);
			blocks = (u32 *)buf3->b_data;
			for (u32 k = 0; k < 256; ++k) {
				if (!blocks[k]) {
					continue;
				}
				free_block(inode->i_dev, blocks[k]);
				blocks[k] = 0;
				inode->i_blocks -= 2;
			}
			write_blk(buf3);
			//rw_block(WRITE, inode->i_dev, indirect[j], &buf3);
			free_block(inode->i_dev, indirect[j]);
			indirect[j] = 0;
			inode->i_blocks -= 2;

			free(buf3->b_data);
			free(buf3);
		}
		write_blk(buf2);
		//rw_block(WRITE, inode->i_dev, dindirect[i], &buf2);
		free_block(inode->i_dev, dindirect[i]);
		dindirect[i] = 0;
		inode->i_blocks -= 2;

		free(buf2->b_data);
		free(buf2);
	}
	write_blk(buf);
	//rw_block(WRITE, inode->i_dev, inode->i_block[14], &buf);
	free_block(inode->i_dev, inode->i_block[14]);
	inode->i_block[14] = 0;
	inode->i_blocks -= 2;

	free(buf->b_data);
	free(buf);
}

void ext2_truncate(struct ext2_inode *inode) {
	trunc_direct(inode);
	trunc_indirect(inode);
	trunc_doubly_indirect(inode);
	trunc_triply_indirect(inode);
	inode->i_size = 0;
	inode->i_dirt = 1;
	inode->i_mtime = inode->i_ctime = get_current_time();
}

