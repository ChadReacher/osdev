#ifndef EXT2_H
#define EXT2_H

#include "types.h"
#include "vfs.h"

#define EXT2_SUPERMAGIC 0xEF53

typedef struct {
	u32 total_inodes;
	u32 total_blocks;
	u32 reserved_blocks_number;
	u32 total_free_blocks;
	u32 total_free_inodes;
	u32 first_data_block;
	u32 log_block_size;
	u32 log_fragment_size;
	u32 blocks_per_group;
	u32 fragments_per_group;
	u32 inodes_in_block_group;
	u32 last_mount_time;
	u32 last_written_time;
	u16 mount_times;
	u16 max_mount_times;
	u16 ext2_super_magic;
	u16 file_system_state;
	u16 errors;
	u16 minor_version;
	u32 last_check;
	u32 check_interval;
	u32 creator_os;
	u32 major_version;
	u16 default_uid_reserved_blocks;
	u16 default_gid_reserved_blocks;

	// Extended Superblock Fields(if major version >= 1)
	u32 first_inode;
	u16 inode_size;
	u16 block_group_part_of;
	u32 feature_compatible;
	u32 feature_incompatible;
	u32 feature_ro_compatible;
	u8 uuid[16];
	i8 volume_name[16];
	i8 last_mounted_path_volume[64];
	u32 compression_algorithm_bitmap;
	u8 blocks_to_preallocate_for_files;
	u8 blocks_to_preallocate_for_dirs;
	u16 _padding;

	// Journaling Support
	u8 journal_id[16];
	u32 journal_inode;
	u32 journal_device;
	u32 last_orphan;

	// Directory Indexing Support
	u32 hash_seed[4];
	u8 def_hash_version;
	u16 _padding_a;
	u8 _padding_b;

	// Other options
	u32 defaul_mount_options;
	u32 first_meta_bg;
	u8 _unused[760];

} __attribute__((packed)) ext2_super_block;

typedef struct {
	u32 block_bitmap;
	u32 inode_bitmap;
	u32 inode_table;
	u16 free_blocks_count;
	u16 free_inodes_count;
	u16 used_dirs_count;
	u16 _padding;
	u8 reserved[12];
} __attribute__((packed)) ext2_block_group_descriptor;

typedef struct {
	u16 mode;
	u16 uid;
	u32 size;			// size of file in bytes
	u32 atime;
	u32 ctime;
	u32 mtime;
	u32 dtime;
	u16 gid;
	u16 links_count;
	u32 blocks;
	u32 flags;
	u32 osd1;
	u32 block[15];
	u32 generation;
	u32 file_acl;
	u32 dir_acl;
	u32 faddr;
	u8 osd2[12];
} __attribute__((packed)) ext2_inode_table;

// File Format
#define EXT2_S_IFSOCK 0xC000
#define EXT2_S_IFLNK 0xA000
#define EXT2_S_IFREG 0x8000
#define EXT2_S_IFBLK 0x6000
#define EXT2_S_IFDIR 0x4000
#define EXT2_S_IFCHR 0x2000
#define EXT2_S_IFIFO 0x1000

// Process execution user/group override

#define EXT2_S_ISUID 0x0800
#define EXT2_S_ISGID 0x0400
#define EXT2_S_ISVTX 0x0200

// Access rights
#define EXT2_S_IRUSR 0x0100
#define EXT2_S_IWUSR 0x0080
#define EXT2_S_IXUSR 0x0040
#define EXT2_S_IRGRP 0x0020
#define EXT2_S_IWGRP 0x0010
#define EXT2_S_IXGRP 0x0008
#define EXT2_S_IROTH 0x0004
#define EXT2_S_IWOTH 0x0002
#define EXT2_S_IXOTH 0x0001


#define POINTERS_PER_BLOCK 256

typedef struct {
	u32 inode;
	u16 rec_len;
	u8 name_len;
	u8 file_type;
	u8 name[];
} ext2_dir;

typedef struct {
	vfs_node_t *disk_device;
	ext2_super_block *superblock;
	ext2_block_group_descriptor *bgd_table;
	u32 block_size;
	u32 blocks_per_group;
	u32 inodes_per_group;
	u32 total_groups;
	u32 block_group_descs;
} ext2_fs;

void ext2_init(i8 *device_path, i8 *mountpoint);
u32 ext2_read(vfs_node_t *node, u32 offset, u32 size, i8 *buffer);
u32 ext2_write(vfs_node_t *node, u32 offset, u32 size, i8 *buffer);
u32 ext2_open(vfs_node_t *node, u32 flags);
u32 ext2_close(vfs_node_t *node);
void ext2_create(vfs_node_t *node, i8 *name, u16 permission);
dirent *ext2_readdir(vfs_node_t *node, u32 index);
vfs_node_t *ext2_finddir(vfs_node_t *node, i8 *name);

ext2_inode_table *ext2_get_inode_table(u32 inode_num);
void ext2_set_inode_table(ext2_inode_table *inode, u32 inode_num);

vfs_node_t *ext2_make_vfs_node(ext2_inode_table *root_inode);
u32 get_real_block(ext2_inode_table *inode, u32 block_num);

u32 ext2_read_inode_filedata(ext2_inode_table *inode, u32 offset, u32 size, i8 *buffer);
void ext2_write_inode_filedata(ext2_inode_table *inode, u32 inode_idx, u32 offset, u32 size, i8 *buffer);

void ext2_create_vfs_node_from_file(ext2_inode_table *inode, ext2_dir *found_dirent, vfs_node_t *vfs_node);

void read_inode_disk_block(ext2_inode_table *inode, u32 block, i8 *buf);
void write_inode_disk_block(ext2_inode_table *inode, u32 block, i8 *buf);

u32 inode_alloc();
u32 block_alloc();

void ext2_create_dir_entry(vfs_node_t *parent, i8 *name, u32 inode_idx);

#endif
