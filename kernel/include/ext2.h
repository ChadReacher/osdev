#ifndef EXT2_H
#define EXT2_H

#include <types.h>
#include <syscall.h>

#define EXT2_SUPER_MAGIC 0xEF53

#define EXT2_NAME_LEN 255
#define EXT2_DIR_PAD                    4
#define EXT2_DIR_ROUND                  (EXT2_DIR_PAD - 1)
#define EXT2_DIR_REC_LEN(namelen)       (((namelen) + 8 + EXT2_DIR_ROUND) & \
                                        ~EXT2_DIR_ROUND)

#define MAY_EXEC  1
#define MAY_WRITE 2
#define MAY_READ  4

#define EXT2_ROOT_INO 2

#define EXT2_S_IFSOCK 0xC000
#define EXT2_S_IFLNK  0xA000
#define EXT2_S_IFREG  0x8000
#define EXT2_S_IFBLK  0x6000
#define EXT2_S_IFDIR  0x4000
#define EXT2_S_IFCHR  0x2000
#define EXT2_S_IFIFO  0x1000

#define EXT2_S_ISREG(m) (((m) & EXT2_S_IFREG) == EXT2_S_IFREG)
#define EXT2_S_ISDIR(m) (((m) & EXT2_S_IFDIR) == EXT2_S_IFDIR)
#define EXT2_S_ISCHR(m) (((m) & EXT2_S_IFCHR) == EXT2_S_IFCHR)
#define EXT2_S_ISBLK(m) (((m) & EXT2_S_IFBLK) == EXT2_S_IFBLK)
#define EXT2_S_ISLNK(m) (((m) & EXT2_S_IFLNK) == EXT2_S_IFLNK)
#define EXT2_S_ISFIFO(m) (((m) & EXT2_S_IFIFO) == EXT2_S_IFIFO)

#define EXT2_S_ISUID 0x0800
#define EXT2_S_ISGID 0x0400
#define EXT2_S_ISVTX 0x0200

#define EXT2_S_IRUSR 0x0100
#define EXT2_S_IWUSR 0x0080
#define EXT2_S_IXUSR 0x0040
#define EXT2_S_IRGRP 0x0020
#define EXT2_S_IWGRP 0x0010
#define EXT2_S_IXGRP 0x0008
#define EXT2_S_IROTH 0x0004
#define EXT2_S_IWOTH 0x0002
#define EXT2_S_IXOTH 0x0001


#define EXT2_N_BLOCKS 15
#define EXT2_DIRECT_BLOCKS 12
#define EXT2_POINTERS_PER_BLOCK 256

struct ext2_super_block {
    u32 s_inodes_count;
    u32 s_blocks_count;
    u32 s_r_blocks_count;
    u32 s_free_blocks_count;
    u32 s_free_inodes_count;
    u32 s_first_data_block;
    u32 s_log_block_size;
    u32 s_log_frag_size;
    u32 s_blocks_per_group;
    u32 s_frags_per_group;
    u32 s_inodes_per_group;
    u32 s_mtime;
    u32 s_wtime;
    u16 s_mnt_count;
    u16 s_max_mnt_count;
    u16 s_magic;
    u16 s_state;
    u16 s_errors;
    u16 s_minor_rev_level;
    u32 s_lastcheck;
    u32 s_checkinterval;
    u32 s_creator_os;
    u32 s_rev_level;
    u16 s_def_resuid;
    u16 s_def_resgid;

    /* Extended Superblock Fields(if major version >= 1) */
    u32 s_first_ino;
    u16 s_inode_size;
    u16 s_block_group_nr;
    u32 s_feature_compat;
    u32 s_feature_incompat;
    u32 s_feature_ro_compat;
    u8  s_uuid[16];
    i8  s_volume_name[16];
    i8  s_last_mounted[64];
    u32 s_algo_bitmap;
    u8  s_prealloc_blocks;
    u8  s_prealloc_dir_blocks;
    u16 s_padding1;

    /* Journaling Support */
    u8  s_journal_uuid[16];
    u32 s_journal_inum;
    u32 s_journal_dev;
    u32 s_last_orphan;

    /* Directory Indexing Support */
    u32 s_hash_seed[4];
    u8  s_def_hash_version;
    u16 s_padding_a;
    u8  s_padding_b;

    /* Other options */
    u32 s_defaul_mount_options;
    u32 s_first_meta_bg;
    u8  s_unused[760];
} __attribute__((packed));

struct ext2_blk_grp_desc {
    u32 bg_block_bitmap;
    u32 bg_inode_bitmap;
    u32 bg_inode_table;
    u16 bg_free_blocks_count;
    u16 bg_free_inodes_count;
    u16 bg_used_dirs_count;
    u16 bg_padding;
    u8  bg_reserved[12];
} __attribute__((packed));

struct ext2_inode {
    u16 i_mode;
    u16 i_uid;
    i32 i_size;
    u32 i_atime;
    u32 i_ctime;
    u32 i_mtime;
    u32 i_dtime; // not used
    u16 i_gid;
    u16 i_links_count;
    u32 i_blocks;
    u32 i_flags;
    u32 i_osd1; // not used
    u32 i_block[EXT2_N_BLOCKS];
    u32 i_generation; // not used
    u32 i_file_acl; // not used
    u32 i_dir_acl; // not used
    u32 i_faddr; // not used
    u8  osd2[12]; // not used

} __attribute__((packed));

struct ext2_dir {
    u32 inode;
    u16 rec_len;
    u16 name_len;
    i8  name[EXT2_NAME_LEN];
} __attribute__((packed));

struct vfs_inode;
struct file;
struct vfs_superblock;

i32 ext2_read_super(struct vfs_superblock *vsb);
i32 ext2_write_super(struct vfs_superblock *vsb);

void ext2_read_inode(struct vfs_inode *inode);
void ext2_write_inode(struct vfs_inode *inode);
void ext2_free_inode(struct vfs_inode *inode);

i32 ext2_create(struct vfs_inode *dir, const i8 *name, i32 mode, struct vfs_inode **result);
i32 ext2_rename(struct vfs_inode *old_dir, const i8 *old_base, struct vfs_inode *new_dir, const i8 *new_base);
i32 ext2_file_read(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
i32 ext2_file_write(struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
i32 ext2_chr_open(struct vfs_inode *inode, struct file *fp);
i32 ext2_blk_open(struct vfs_inode *inode, struct file *fp);

i32 ext2_unlink(struct vfs_inode *dir, const char *basename);
i32 ext2_link(struct vfs_inode *dir, const i8 *basename, struct vfs_inode *inode);
i32 ext2_symlink(struct vfs_inode *dir, const i8 *name, const i8*symname);
i32 ext2_readlink(struct vfs_inode *inode, i8 *buf, i32 bufsiz);
i32 ext2_follow_link(struct vfs_inode *inode, struct vfs_inode *base, struct vfs_inode **res);
i32 ext2_rmdir(struct vfs_inode *dir, const char *basename);
i32 ext2_mkdir(struct vfs_inode *dir, const char *basename, i32 mode);

struct buffer *ext2_find_entry(struct vfs_inode *dir, const i8 *name, struct ext2_dir **res_dir, struct ext2_dir **prev_dir);
i32 ext2_readdir(struct vfs_inode *inode, struct file *fp, struct dirent *dent);

i32 ext2_lookup(struct vfs_inode *dir, const i8 *name, struct vfs_inode **res);
i32 ext2_truncate(struct vfs_inode *inode, u32 length);
i32 ext2_mount(struct vfs_inode *dir, u32 dev, const char *basename);


// Low-level API
i32 read_group_desc(struct ext2_blk_grp_desc *bgd, u32 group, struct vfs_superblock *vsb);
i32 write_group_desc(struct ext2_blk_grp_desc *bgd, u32 group, struct vfs_superblock *vsb);
u32 ext2_alloc_block(u16 dev);
void ext2_free_block(u16 dev, u32 block);
struct vfs_inode *ext2_alloc_inode(u16 dev);
void ext2_free_inode(struct vfs_inode *inode);

i32 ext2_add_entry(struct vfs_inode *dir, const i8 *name, struct buffer **res_buf, struct ext2_dir **result);

i32 ext2_bmap(struct vfs_inode *inode, u32 offset);
i32 ext2_create_block(struct vfs_inode *inode, u32 offset);



#endif
