#include <ext2.h>
#include <process.h>
#include <string.h>
#include <debug.h>
#include <panic.h>
#include <blk_dev.h>

extern process_t *current_process;
extern struct ext2_super_block super_block;

i32 bmap(struct ext2_inode *inode, u32 offset) {
	i8 *buf;
	u32 *indirect;
	u32 block;

	block = offset / super_block.s_block_size;
	if (block < 12) {
		DEBUG("Direct block\r\n");
		return inode->i_block[block];
	} else if (block < 12 + EXT2_POINTERS_PER_BLOCK) {
		DEBUG("Indirect block\r\n");
		block -= 12;
		rw_block(READ, inode->i_dev, inode->i_block[13], &buf);
		u32 res_block = ((u32 *)buf)[block];
		free(buf);
		return res_block;
	} else if (block < 12 + EXT2_POINTERS_PER_BLOCK + EXT2_POINTERS_PER_BLOCK *
			EXT2_POINTERS_PER_BLOCK) {
		DEBUG("Doubly-indirect block\r\n");
		block -= 12;
		block -= EXT2_POINTERS_PER_BLOCK;
		rw_block(READ, inode->i_dev, inode->i_block[13], &buf);
		u32 ind_block = ((u32 *)buf)[block / EXT2_POINTERS_PER_BLOCK];
		free(buf);
		rw_block(READ, inode->i_dev, ind_block, &buf);
		u32 res_block = ((u32 *)buf)[block % EXT2_POINTERS_PER_BLOCK];
		free(buf);
		return res_block;
	} else if (block < 12 + EXT2_POINTERS_PER_BLOCK + EXT2_POINTERS_PER_BLOCK *
			EXT2_POINTERS_PER_BLOCK + EXT2_POINTERS_PER_BLOCK *
			EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK) {
		DEBUG("Triply-indirect block\r\n");
		block -= 12;
		block -= EXT2_POINTERS_PER_BLOCK;
		block -= EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK;
		rw_block(READ, inode->i_dev, inode->i_block[14], &buf);
		u32 dind_block = ((u32 *)buf)[block / (EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK)];
		free(buf);
		rw_block(READ, inode->i_dev, dind_block, &buf);
		u32 ind_block = ((u32 *)buf)[block % (EXT2_POINTERS_PER_BLOCK * EXT2_POINTERS_PER_BLOCK)];
		u32 ind_block2 = ind_block / EXT2_POINTERS_PER_BLOCK;
		free(buf);
		rw_block(READ, inode->i_dev, ind_block2, &buf);
		u32 res_block = ((u32 *)buf)[ind_block % EXT2_POINTERS_PER_BLOCK];
		free(buf);
		return res_block;
	}
	return 0;
}

i32 find_entry(struct ext2_inode *dir, i8 *name, struct ext2_dir **res_dir) {
	i8 *buf, *dname;
	struct ext2_dir *de;
	u32 curr_off, block, inblock_offset;

	curr_off = block = inblock_offset = 0;

	rw_block(READ, dir->i_dev, dir->i_block[block], &buf);
	while (curr_off < dir->i_size) {
		if (inblock_offset >= super_block.s_block_size) {
			DEBUG("Going to the next block\r\n");
			block = bmap(dir, curr_off);
			inblock_offset = 0;
			free(buf);
			rw_block(READ, dir->i_dev, block, &buf);
		}
		de = (struct ext2_dir *)(buf + inblock_offset);
		dname = malloc(de->name_len + 1);
		memcpy(dname, de->name, de->name_len);
		dname[de->name_len] = '\0';
		if (strcmp(name, dname) == 0) {
			*res_dir = de;
			return 0;
		}

		DEBUG("inode - %d, rec_len - %d, name_len - %d, name - %s\r\n",
			  de->inode, de->rec_len, de->name_len, dname);
		free(dname);

		inblock_offset += de->rec_len;
		curr_off += de->rec_len;
	}
	return 1;
}

struct ext2_inode *namei(const i8 *pathname) {
	DEBUG("START\r\n");
	DEBUG("pathname: %s\r\n", pathname);
	struct ext2_inode *inode;
	struct ext2_dir *de;
	i8 *thisname, *basename, *pathname_dup, *saved_pathname, *name;
	u32 inr, idev, namelen;

	pathname_dup = strdup(pathname);
	saved_pathname = pathname_dup;

	/*
	if (!current_process->root || !current_process->root->i_count) {
		PANIC("No root inode\r\n");
	}
	if (!current_process->pwd || !current_process->pwd->i_count) {
		PANIC("No cwd inode\r\n");
	}
	*/
	if (!pathname) {
		return NULL;
	}
	/*
	if (pathname[0] == '/') {
		inode = current_process->root;
		++pathname;
	} else {
		inode = current_process->pwd;
	}
	*/
	inode = iget(ROOT_DEV, 2);
	++pathname_dup;


	// /usr/file
	++inode->i_count;
	while ((name = strsep(&pathname_dup, "/")) != NULL) {
		DEBUG("name - %s, len - %d\r\n", name, strlen(name));

		// If it is not a dir
		if (!EXT2_S_ISDIR(inode->i_mode)) {
			iput(inode);
			return NULL;
		}

		// TODO: Check permission

		if (find_entry(inode, name, &de)) {
			iput(inode);
			return NULL;
		}
		inr = de->inode;
		idev = inode->i_dev;
		iput(inode);
		if (!(inode = iget(idev, inr))) {
			return NULL;
		}
	}
	DEBUG("END\r\n");
	return inode;
}
