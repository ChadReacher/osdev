#include "ext2.h"
#include "heap.h"
#include "vfs.h"
#include "debug.h"
#include "string.h"
#include "memory.h"

ext2_fs *ext2fs = NULL;

static void read_disk_block(u32 block, i8 *buf);
static void write_disk_block(u32 block, i8 *buf);

void ext2_init(i8 *device_path, i8 *mountpoint) {
	ext2fs = malloc(sizeof(ext2_fs));

	vfs_node_t *disk_device = vfs_get_node(device_path);
	if (disk_device == NULL) {
		DEBUG("Could not find the device - %s\r\n.", device_path);
		return;
	}
	ext2fs->disk_device = disk_device;
	ext2fs->superblock = malloc(sizeof(ext2_super_block));
	ext2fs->block_size = 1024; // For now it will be 1024. Just to read superblock
	read_disk_block(1, (void *)ext2fs->superblock);
	if (ext2fs->superblock->ext2_super_magic != EXT2_SUPERMAGIC) {
		DEBUG("%s", "It is not an EXT2 file system.\r\n");
		return;
	}
	DEBUG("%s", "It IS an EXT2 file system\r\n");

	ext2fs->block_size = 1024 << ext2fs->superblock->log_block_size;
	ext2fs->blocks_per_group = ext2fs->superblock->blocks_per_group;
	ext2fs->inodes_per_group = ext2fs->superblock->inodes_in_block_group;
	ext2fs->total_groups = ext2fs->superblock->total_blocks / ext2fs->blocks_per_group;
	ext2fs->block_group_descs = ext2fs->total_groups;
	u32 block_group_descs_in_blocks = (ext2fs->block_group_descs * sizeof(ext2_block_group_descriptor)) / ext2fs->block_size + 1;

	DEBUG("revision level - %x\r\n", ext2fs->superblock->major_version);
	DEBUG("block_size = %x\r\n", ext2fs->block_size);
	DEBUG("blocks_per_group = %x\r\n", ext2fs->blocks_per_group);
	DEBUG("inodes_per_group = %x\r\n", ext2fs->inodes_per_group);
	DEBUG("total_blocks = %x\r\n", ext2fs->superblock->total_blocks);
	DEBUG("total_groups = %x\r\n", ext2fs->total_groups);
	DEBUG("We have %x bgds.\r\n", ext2fs->block_group_descs);
	DEBUG("block_group_descs_in_blocks = %x\r\n", block_group_descs_in_blocks);
	DEBUG("inode size = %x\r\n", ext2fs->superblock->inode_size);

	ext2fs->bgd_table = malloc(block_group_descs_in_blocks * ext2fs->block_size);

	DEBUG("bgd_table = %x\r\n", ext2fs->bgd_table);
	DEBUG("first inode - %x\r\n", ext2fs->superblock->first_inode);

	for (u32 i = 0; i < block_group_descs_in_blocks; ++i) {
		read_disk_block(2 + i, (((i8 *)ext2fs->bgd_table) + i * ext2fs->block_size));
	}

	ext2_inode_table *root_inode = ext2_get_inode_table(2);
	DEBUG("%s", "Root inode: \r\n");
	DEBUG("mode = %x\r\n", root_inode->mode);
	DEBUG("uid = %x\r\n", root_inode->uid);
	DEBUG("size = %x\r\n", root_inode->size);
	DEBUG("gid = %x\r\n", root_inode->gid);
	DEBUG("links_count = %x\r\n", root_inode->links_count);
	DEBUG("blocks = %x\r\n", root_inode->blocks);
	DEBUG("flags = %x\r\n", root_inode->flags);
	
	vfs_node_t *ext2_vfs_node = ext2_make_vfs_node(root_inode);
	vfs_mount(mountpoint, ext2_vfs_node);
}

u32 ext2_read(vfs_node_t *node, u32 offset, u32 size, i8 *buffer) {
	ext2_inode_table *inode = ext2_get_inode_table(node->inode);
	u32 have_read = ext2_read_inode_filedata(inode, offset, size, buffer);
	return have_read;
}

u32 ext2_write(vfs_node_t *node, u32 offset, u32 size, i8 *buffer) {
	ext2_inode_table *inode = ext2_get_inode_table(node->inode);	
	ext2_write_inode_filedata(inode, node->inode, offset, size, buffer);
	return size;
}

void ext2_write_inode_filedata(ext2_inode_table *inode, u32 inode_idx, u32 offset, u32 size, i8 *buffer) {
	// If we are increasing the file size, 
	// then rewrite inode size
	if (offset + size > inode->size) {
		inode->size = offset + size;
		ext2_set_inode_table(inode, inode_idx);
	}
	u32 start_block = offset / ext2fs->block_size;
	u32 end_offset = (offset + size <= inode->size) ? (offset + size) : (inode->size);
	u32 end_block = end_offset / ext2fs->block_size;

	u32 start_off = offset % ext2fs->block_size;
	u32 end_size = end_offset - end_block * ext2fs->block_size;

	u32 i = start_block;
	u32 buf_off = 0;
	i8 *block_buf = malloc(ext2fs->block_size);
	while (i <= end_block) {
		u32 left = 0, right = ext2fs->block_size;
		read_inode_disk_block(inode, i, block_buf);
		if (i == start_block) {
			left = start_off;
		}
		if (i == end_block) {
			right = end_size - 1;
		}
		memcpy(block_buf + left, buffer + buf_off, (right - left + 1));
		buf_off += (right - left + 1);
		write_inode_disk_block(inode, i, block_buf);
		++i;

	}

	free(block_buf);
}

u32 ext2_read_inode_filedata(ext2_inode_table *inode, u32 offset, u32 size, i8 *buffer) {
	u32 start_block = offset / ext2fs->block_size;
	u32 end_offset = (offset + size <= inode->size) ? (offset + size) : (inode->size);
	u32 end_block = end_offset / ext2fs->block_size;

	u32 start_off = offset % ext2fs->block_size;
	u32 end_size = end_offset - end_block * ext2fs->block_size;

	u32 i = start_block;
	u32 buf_off = 0;
	i8 *block_buf = malloc(ext2fs->block_size);
	while (i <= end_block) {
		u32 left = 0, right = ext2fs->block_size - 1;
		read_inode_disk_block(inode, i, block_buf);
		if (i == start_block) {
			left = start_off;
		}
		if (i == end_block) {
			right = end_size - 1;
		}
		memcpy(buffer + buf_off, block_buf + left, (right - left + 1));
		buf_off += (right - left + 1);
		++i;
	}
	free(block_buf);
	return end_offset - offset;
}

void read_inode_disk_block(ext2_inode_table *inode, u32 block, i8 *buf) {
	u32 real_block = get_real_block(inode, block);
	read_disk_block(real_block, buf);
}

void write_inode_disk_block(ext2_inode_table *inode, u32 block, i8 *buf) {
	u32 real_block = get_real_block(inode, block);
	write_disk_block(real_block, buf);
}

u32 get_real_block(ext2_inode_table *inode, u32 block_num) {
	if (block_num < 12) {
		return inode->block[block_num];
	} else if (block_num < 12 + POINTERS_PER_BLOCK) {
		i8 *tmp = malloc(ext2fs->block_size);
		read_disk_block(inode->block[12], tmp);
		u32 nblock = ((u32 *)tmp)[block_num - 12];

		free(tmp);
		return nblock;
	} else if (block_num < 12 + POINTERS_PER_BLOCK + POINTERS_PER_BLOCK * POINTERS_PER_BLOCK) {
		u32 a = block_num - 12;
		u32 b = a - POINTERS_PER_BLOCK;
		u32 c = b / POINTERS_PER_BLOCK;

		i8 *tmp = malloc(ext2fs->block_size);
		read_disk_block(inode->block[13], tmp);

		u32 nblock = ((u32 *)tmp)[c];
		read_disk_block(nblock, tmp);

		u32 d = b - c * POINTERS_PER_BLOCK;
		nblock = ((u32 *)tmp)[d];

		free(tmp);
		return nblock;
	} else if (block_num < 12 + POINTERS_PER_BLOCK + POINTERS_PER_BLOCK * POINTERS_PER_BLOCK + POINTERS_PER_BLOCK * POINTERS_PER_BLOCK * POINTERS_PER_BLOCK) {
		u32 a = block_num - 12;
		u32 b = a - POINTERS_PER_BLOCK;
		u32 c = b - POINTERS_PER_BLOCK * POINTERS_PER_BLOCK;

		i8 *tmp = malloc(ext2fs->block_size);
		read_disk_block(inode->block[14], tmp);

		u32 d = c / (POINTERS_PER_BLOCK * POINTERS_PER_BLOCK);
		u32 nblock = ((u32 *)tmp)[d];
		read_disk_block(nblock, tmp);

		u32 e = c - d * POINTERS_PER_BLOCK * POINTERS_PER_BLOCK;
		u32 f = e / POINTERS_PER_BLOCK;
		nblock = ((u32 *)tmp)[f];
		read_disk_block(nblock, tmp);

		u32 g = e - f * POINTERS_PER_BLOCK;
		nblock = ((u32 *)tmp)[g];

		free(tmp);
		return nblock;
	}
	return 0;
}

u32 ext2_open(vfs_node_t *node, u32 flags) {
	(void)node;
	(void)flags;
	return 0;
}

void set_real_block(ext2_inode_table *inode, u32 inode_idx, u32 inode_block, u32 disk_block) {
	i32 a, b, c, d, e, f, g;
	i32 iblock = inode_block;
	u32 *tmp = malloc(ext2fs->block_size);

	a = iblock - 12;
	if (a <= 0) {
		inode->block[inode_block] = disk_block;

		free(tmp);
		return;
	} 
	b = a - 256;
	if (b <= 0) {
		// If the inode block entry is not allocated
		if (*(&inode->block[12]) == 0) {
			u32 block_num = block_alloc();
			inode->block[12] = block_num;
			ext2_set_inode_table(inode, inode_idx);
		}
		read_disk_block(inode->block[12], (i8 *)tmp);
		((u32*)tmp)[a] = disk_block;
		write_disk_block(inode->block[12], (i8 *)tmp);
		tmp[a] = disk_block;

		free(tmp);
		return;
	}
	c = b - 256 * 256;
	if (c <= 0) {
		c = b / 256;
		d = b - c * 256;

		if (*(&inode->block[13]) == 0) {
			u32 block_num = block_alloc();
			inode->block[13] = block_num;
			ext2_set_inode_table(inode, inode_idx);
		}
		read_disk_block(inode->block[13], (i8 *)tmp);

		if (*(&tmp[c]) == 0) {
			u32 block_num = block_alloc();
			tmp[c] = block_num;
			write_disk_block(inode->block[13], (i8 *)tmp);
		}
		u32 temp = tmp[c];
		read_disk_block(temp, (i8 *)tmp);
		tmp[d] = disk_block;
		write_disk_block(temp, (i8 *)tmp);

		free(tmp);
		return;
	} 
	d = c - 256 * 256 * 256;
	if (d <= 0) {
		e = c / (256 * 256);
		f = (c - e * 256 * 256) / 256;
		g = c - e * 256 * 256 - f * 256;

		if (*(&inode->block[14]) == 0) {
			u32 block_num = block_alloc();
			inode->block[14] = block_num;
			ext2_set_inode_table(inode, inode_idx);
		}
		read_disk_block(inode->block[14], (i8 *)tmp);

		if (*(&tmp[e]) == 0) {
			u32 block_num = block_alloc();
			tmp[e] = block_num;
			write_disk_block(inode->block[14], (i8 *)tmp);
		}
		u32 temp = tmp[e];
		read_disk_block(temp, (i8 *)tmp);

		if (*(&tmp[f]) == 0) {
			u32 block_num = block_alloc();
			tmp[f] = block_num;
			write_disk_block(temp, (i8 *)tmp);
		}
		temp = tmp[f];
		read_disk_block(temp, (i8 *)tmp);
		tmp[g] = disk_block;
		write_disk_block(temp, (i8 *)tmp);

		free(tmp);
		return;
	}

	free(tmp);
}

u32 ext2_close(vfs_node_t *node) {
	(void)node;
	return 0;
}

// Caller can free return value.
dirent *ext2_readdir(vfs_node_t *node, u32 index) {
	ext2_inode_table *inode = ext2_get_inode_table(node->inode);
	if (!(inode->mode & EXT2_S_IFDIR)) {
		DEBUG("%s", "It is not a directory\r\n");
		return NULL;
	}

	u32 curr_offset = 0;
	u32 block_offset = 0;
	u32 in_block_offset = 0;
	u32 dir_entry_idx = 0;

	i8 *block_buf = malloc(ext2fs->block_size);
	read_inode_disk_block(inode, block_offset, block_buf);

	while (curr_offset < inode->size) {
		if (in_block_offset >= ext2fs->block_size) {
			++block_offset;
			in_block_offset = 0;
			read_inode_disk_block(inode, block_offset, block_buf);
		}

		ext2_dir *dir_entry = (ext2_dir *)(block_buf + in_block_offset);

		if (dir_entry_idx == index) {
			dirent *vfs_dir_entry = malloc(sizeof(dirent));
			memcpy(vfs_dir_entry->name, (i8 *)dir_entry->name, dir_entry->name_len);
			vfs_dir_entry->name[dir_entry->name_len] = '\0';
			vfs_dir_entry->inode = dir_entry->inode;

			free(block_buf);
			free(inode);
			return vfs_dir_entry;
		}
	
		in_block_offset += dir_entry->rec_len;
		curr_offset += dir_entry->rec_len;
		++dir_entry_idx;
	}

	free(block_buf);
	free(inode);
	return NULL;
}

vfs_node_t *ext2_finddir(vfs_node_t *node, i8 *name) {
	ext2_inode_table *inode = ext2_get_inode_table(node->inode);
	if (!(inode->mode & EXT2_S_IFDIR)) {
		DEBUG("%s", "It is not a directory\r\n");
		return NULL;
	}
	ext2_dir *found_dirent = NULL;

	u32 curr_offset = 0;
	u32 block_offset = 0;
	u32 in_block_offset = 0;

	i8 *block_buf = malloc(ext2fs->block_size);
	read_inode_disk_block(inode, block_offset, block_buf);

	while (curr_offset < inode->size) {
		ext2_dir *dir_entry = (ext2_dir *)(block_buf + in_block_offset);

		if (strlen(name) != dir_entry->name_len) {
			in_block_offset += dir_entry->rec_len;
			curr_offset += in_block_offset;

			if (in_block_offset >= ext2fs->block_size) {
				++block_offset;
				in_block_offset = 0;
				read_inode_disk_block(inode, block_offset, block_buf);
			}	
			continue;
		}

		i8 *dname = malloc(dir_entry->name_len + 1);
		memcpy(dname, dir_entry->name, dir_entry->name_len);
		dname[dir_entry->name_len] = '\0';
		if (strcmp(dname, name) == 0) {
			free(dname);
			found_dirent = malloc(dir_entry->rec_len);
			memcpy(found_dirent, dir_entry, dir_entry->rec_len);
			break;
		}
		free(dname);

		in_block_offset += dir_entry->rec_len;
		curr_offset += in_block_offset;

		if (in_block_offset >= ext2fs->block_size) {
			++block_offset;
			in_block_offset = 0;
			read_inode_disk_block(inode, block_offset, block_buf);
		}
	}

	free(block_buf);
	free(inode);

	if (!found_dirent) {
		return NULL;
	}

	vfs_node_t *vfs_node = malloc(sizeof(vfs_node_t));
	inode = ext2_get_inode_table(found_dirent->inode);

	ext2_create_vfs_node_from_file(inode, found_dirent, vfs_node);

	free(found_dirent);
	free(inode);
	return vfs_node;
}

void ext2_create(vfs_node_t *parent, i8 *name, u16 permission) {
	u32 inode_idx = inode_alloc(); // alloc inode in inode bitmap
	ext2_inode_table *inode = ext2_get_inode_table(inode_idx);
	inode->mode = EXT2_S_IFREG;
	inode->mode |= 0xFFF & permission;
	inode->atime = 0;
	inode->ctime = 0;
	inode->mtime = 0;
	inode->dtime = 0;
	inode->gid = 0;
	inode->uid = 0;
	inode->faddr = 0;
	inode->size = 0;
	inode->blocks = 0;
	memset(inode->block, 0, sizeof(inode->block));
	inode->flags = 0;
	inode->osd1 = 0;
	inode->generation = 0;
	inode->file_acl = 0;
	inode->dir_acl = 0;
	memset(inode->osd2, 0, sizeof(inode->osd2));

	// Alloc inode block
	u32 inode_block_num = block_alloc(); // alloc block in block bitmap
	set_real_block(inode, inode_idx, 0, inode_block_num); 
	inode->blocks = 2; 
	ext2_set_inode_table(inode, inode_idx);
	ext2_create_dir_entry(parent, name, inode_idx);

	// Update parent inode's size 
	ext2_inode_table *parent_inode = ext2_get_inode_table(parent->inode);
	parent_inode->links_count++;
	ext2_set_inode_table(parent_inode, parent->inode);
}

void ext2_unlink(vfs_node_t *parent_node, i8 *entry_name) {
	ext2_inode_table *parent_inode = ext2_get_inode_table(parent_node->inode);

	u32 curr_offset = 0;
	u32 block_offset = 0;
	u32 in_block_offset = 0;

	u32 entry_name_len = strlen(entry_name);
	i8 *name_buf_check = malloc(entry_name_len + 1);
	memset(name_buf_check, 0, entry_name_len + 1);

	i8 *block_buf = malloc(ext2fs->block_size);
	read_inode_disk_block(parent_inode, block_offset, block_buf);

	while (curr_offset < parent_inode->size) {
		if (in_block_offset >= ext2fs->block_size) {
			++block_offset;
			in_block_offset = 0;
			read_inode_disk_block(parent_inode, block_offset, block_buf);
		}

		ext2_dir *curr_dir_entry = (ext2_dir *)(block_buf + in_block_offset);

		if (curr_dir_entry->name_len == entry_name_len) {
			memcpy(name_buf_check, curr_dir_entry->name, entry_name_len);
			if (curr_dir_entry->inode != 0 && strcmp(entry_name, name_buf_check) == 0) {
				// For now it is jsut sets inode to zero.
				curr_dir_entry->inode = 0;
				write_inode_disk_block(parent_inode, block_offset, block_buf);
				break;
			}
		}

		u32 expected_size = ((sizeof(ext2_dir) + curr_dir_entry->name_len) & 0xFFFFFFFC) + 0x4;
		u32 real_size = curr_dir_entry->rec_len;
		if (real_size != expected_size) {
			break;
		}
		in_block_offset += curr_dir_entry->rec_len;
		curr_offset += in_block_offset;
	}

	free(parent_inode);
	parent_inode = ext2_get_inode_table(parent_node->inode);
	--parent_inode->links_count;
	ext2_set_inode_table(parent_inode, parent_node->inode);

	free(name_buf_check);
	free(block_buf);
	free(parent_inode);
}

u32 inode_alloc() {
	u32 *buf = malloc(ext2fs->block_size);
	memset(buf, 0, ext2fs->block_size);
	for (u32 i = 0; i < ext2fs->total_groups; ++i) {
		if (!ext2fs->bgd_table[i].free_inodes_count) {
			continue;
		}

		u32 inode_bitmap_block = ext2fs->bgd_table[i].inode_bitmap;
		read_disk_block(inode_bitmap_block, (i8 *)buf);
		for (u32 j = 0; j < ext2fs->block_size / 4; ++j) {
			u32 sub_bitmap = buf[j];
			if (sub_bitmap == 0xFFFFFFFF) {
				continue;
			}
			for (u32 k = 0; k < 32; ++k) {
				u32 is_free = !(sub_bitmap & (1 << k));
				if (is_free) {
					u32 mask = 1 << k;
					buf[j] = buf[j] | mask;
					write_disk_block(inode_bitmap_block, (i8 *)buf);
					--ext2fs->bgd_table[i].free_inodes_count;
					
					free(buf);
					return i * ext2fs->inodes_per_group + j * 32 + k;
				}
			}
		}
	}
	free(buf);
	return (u32) -1;
}

u32 block_alloc() {
	u32 *buf = malloc(ext2fs->block_size);
	memset(buf, 0, ext2fs->block_size);
	for (u32 i = 0; i < ext2fs->total_groups; ++i) {
		if (!ext2fs->bgd_table[i].free_blocks_count) {
			continue;
		}

		u32 block_bitmap_block = ext2fs->bgd_table[i].block_bitmap;
		read_disk_block(block_bitmap_block, (i8 *)buf);
		for (u32 j = 0; j < ext2fs->block_size / 4; ++j) {
			u32 sub_bitmap = buf[j];
			if (sub_bitmap == 0xFFFFFFFF) {
				continue;
			}
			for (u32 k = 0; k < 32; ++k) {
				u32 is_free = !(sub_bitmap & (1 << k));
				if (is_free) {
					u32 mask = 1 << k;
					buf[j] = buf[j] | mask;
					write_disk_block(block_bitmap_block, (i8 *)buf);
					--ext2fs->bgd_table[i].free_blocks_count;

					free(buf);
					return i * ext2fs->blocks_per_group + j * 32 + k;
				}
			}
		}
	}
	free(buf);
	return (u32) -1;
}

void ext2_create_dir_entry(vfs_node_t *parent, i8 *entry_name, u32 inode_idx) {
	ext2_inode_table *inode = ext2_get_inode_table(parent->inode);
	u32 curr_offset = 0;
	u32 block_offset = 0;
	u32 in_block_offset = 0;
	bool found_last_entry = false;
	u32 entry_name_len = strlen(entry_name);
	i8 *name_buf_check = malloc(entry_name_len + 1);
	memset(name_buf_check, 0, entry_name_len + 1);

	i8 *block_buf = malloc(ext2fs->block_size);
	read_inode_disk_block(inode, block_offset, block_buf);

	while (curr_offset < inode->size) {
		if (in_block_offset >= ext2fs->block_size) {
			++block_offset;
			in_block_offset = 0;
			read_inode_disk_block(inode, block_offset, block_buf);
		}

		ext2_dir *curr_dir_entry = (ext2_dir *)(block_buf + in_block_offset);

		if (curr_dir_entry->name_len == entry_name_len) {
			memcpy(name_buf_check, curr_dir_entry->name, entry_name_len);
			if (curr_dir_entry->inode != 0 && strcmp(entry_name, name_buf_check) == 0) {
				DEBUG("%s", "File with the same name already existed in the directory.\r\n");
				free(name_buf_check);
				free(block_buf);
				free(inode);
				return;
			}
		}

		if (found_last_entry) {
			curr_dir_entry->inode = inode_idx;
			curr_dir_entry->rec_len = (u32)block_buf + ext2fs->block_size - (u32)curr_dir_entry;
			curr_dir_entry->name_len = entry_name_len;
			memcpy(curr_dir_entry->name, entry_name, entry_name_len);
			write_inode_disk_block(inode, block_offset, block_buf);
			// Append a new ending entry
			in_block_offset += curr_dir_entry->rec_len;
			// Check if we are the end of block?
			if (in_block_offset >= ext2fs->block_size) {
				++block_offset;
				in_block_offset = 0;
				read_inode_disk_block(inode, block_offset, block_buf);
			}
			curr_dir_entry = (ext2_dir *)(block_buf + in_block_offset);
			memset(curr_dir_entry, 0, sizeof(ext2_dir));
			write_inode_disk_block(inode, block_offset, block_buf);

			free(name_buf_check);
			free(block_buf);
			free(inode);
			return;
		}


	
		u32 expected_size = ((sizeof(ext2_dir) + curr_dir_entry->name_len) & 0xFFFFFFFC) + 0x4;
		u32 real_size = curr_dir_entry->rec_len;
		if (real_size != expected_size) {
			found_last_entry = true;
			curr_dir_entry->rec_len = expected_size;
			in_block_offset += expected_size;
			curr_offset += in_block_offset;
			continue;
		}
		in_block_offset += curr_dir_entry->rec_len;
		curr_offset += in_block_offset;
	}

	free(name_buf_check);
	free(block_buf);
	free(inode);
}

void ext2_create_vfs_node_from_file(ext2_inode_table *inode, ext2_dir *found_dirent, vfs_node_t *vfs_node) {
	if (!vfs_node) {
		return;
	}
	vfs_node->inode = found_dirent->inode;
	memcpy(&vfs_node->name, &found_dirent->name, found_dirent->name_len);
	vfs_node->name[found_dirent->name_len] = '\0';
	vfs_node->uid = inode->uid;
	vfs_node->gid = inode->gid;
	vfs_node->length = inode->size;
	vfs_node->mask = inode->mode & 0xFFF;
	vfs_node->flags = 0;
	if ((inode->mode & EXT2_S_IFREG) == EXT2_S_IFREG) {
		vfs_node->flags |= FS_FILE;
	} else if ((inode->mode & EXT2_S_IFDIR) == EXT2_S_IFDIR) {
		vfs_node->flags |= FS_DIRECTORY;
	} else if ((inode->mode & EXT2_S_IFBLK) == EXT2_S_IFBLK) {
		vfs_node->flags |= FS_BLOCKDEVICE;
	} else if ((inode->mode & EXT2_S_IFCHR) == EXT2_S_IFCHR) {
		vfs_node->flags |= FS_CHARDEVICE;
	} else if ((inode->mode & EXT2_S_IFIFO) == EXT2_S_IFIFO) {
		vfs_node->flags |= FS_PIPE;
	} else if ((inode->mode & EXT2_S_IFLNK) == EXT2_S_IFLNK) {
		vfs_node->flags |= FS_SYMLINK;
	}

	vfs_node->read = ext2_read;
	vfs_node->write = ext2_write;
	vfs_node->create = ext2_create;
	vfs_node->open = ext2_open;
	vfs_node->close = ext2_close;
	vfs_node->readdir = ext2_readdir;
	vfs_node->finddir = ext2_finddir;
}

static void read_disk_block(u32 block, i8 *buf) {
	vfs_read(ext2fs->disk_device, ext2fs->block_size * block, ext2fs->block_size, buf);
}

static void write_disk_block(u32 block, i8 *buf) {
	vfs_write(ext2fs->disk_device, ext2fs->block_size * block, ext2fs->block_size, buf);
}

// Caller should free the memory
ext2_inode_table *ext2_get_inode_table(u32 inode_num) {
	u32 group = inode_num / ext2fs->inodes_per_group;
	u32 inode_table_block = ext2fs->bgd_table[group].inode_table;

	u32 idx_in_group = inode_num - group * ext2fs->inodes_per_group;
	u32 block_offset = ((idx_in_group - 1) * ext2fs->superblock->inode_size) / ext2fs->block_size;
	u32 offset_in_block = (idx_in_group - 1) - block_offset * (ext2fs->block_size / ext2fs->superblock->inode_size);

	i8 *buf = malloc(ext2fs->block_size);
	ext2_inode_table *inode_table = malloc(ext2fs->superblock->inode_size);

	read_disk_block(inode_table_block + block_offset, buf);

	memcpy(inode_table, (void *)(((u32)buf) + offset_in_block * ext2fs->superblock->inode_size), ext2fs->superblock->inode_size);

	free(buf);
	return inode_table;
}

void ext2_set_inode_table(ext2_inode_table *inode, u32 inode_num) {
	u32 group = inode_num / ext2fs->inodes_per_group;
	u32 inode_table_block = ext2fs->bgd_table[group].inode_table;

	//u32 idx_in_group = inode_num - group * ext2fs->inodes_per_group;
	u32 block_offset = ((inode_num - 1) * ext2fs->superblock->inode_size) / ext2fs->block_size;
	u32 offset_in_block = (inode_num - 1) - block_offset * (ext2fs->block_size / ext2fs->superblock->inode_size);

	i8 *buf = malloc(ext2fs->block_size);

	read_disk_block(inode_table_block + block_offset, buf);
	memcpy(buf + offset_in_block * ext2fs->superblock->inode_size, inode, ext2fs->superblock->inode_size);
	write_disk_block(inode_table_block + block_offset, buf);

	free(buf);
}

vfs_node_t *ext2_make_vfs_node(ext2_inode_table *root_inode) {
	vfs_node_t *ext2_vfs_node = malloc(sizeof(vfs_node_t));
	strcpy(ext2_vfs_node->name, "/");
	ext2_vfs_node->device = ext2fs;
	ext2_vfs_node->mask = root_inode->mode & 0xFFF;
	ext2_vfs_node->uid = root_inode->uid;
	ext2_vfs_node->gid = root_inode->gid;
	ext2_vfs_node->flags |= FS_DIRECTORY;
	ext2_vfs_node->inode = 2;
	ext2_vfs_node->length = root_inode->size;

	ext2_vfs_node->read = ext2_read;
	ext2_vfs_node->write = ext2_write;
	ext2_vfs_node->create = ext2_create;
	ext2_vfs_node->unlink = ext2_unlink;
	ext2_vfs_node->open = ext2_open;
	ext2_vfs_node->close = ext2_close;
	ext2_vfs_node->readdir = ext2_readdir;
	ext2_vfs_node->finddir = ext2_finddir;

	return ext2_vfs_node;
}

