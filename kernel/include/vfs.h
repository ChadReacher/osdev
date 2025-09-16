#ifndef VFS_H
#define VFS_H

#include "types.h"
#include "ext2.h"
#include "pipe.h"

#define ROOT_DEV 0x306

#define S_IFMT   0170000
#define S_IFSOCK 0140000
#define S_IFLNK  0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0006000

#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)

#define NR_FILESYSTEMS 1

#define NR_SUPERBLOCKS 3
#define NR_INODES 32
#define MAX_LINK_COUNT 5

struct vfs_inode;
struct vfs_superblock;
struct file;

// These functions takes ownership of passed 'struct vfs_inode'
// i.e they eat one 'i_count'
struct vfs_inode_ops {
    i32 (*unlink) (struct vfs_inode *dir, const char *basename);
    i32 (*link) (struct vfs_inode *dir, const char *basename, struct vfs_inode *inode);
    i32 (*rmdir) (struct vfs_inode *dir, const char *basename);
    i32 (*mkdir) (struct vfs_inode *dir, const char *basename, i32 mode);
    i32 (*rename) (struct vfs_inode *old_dir, const char *old_base, struct vfs_inode *new_dir, const char *new_base);
    i32 (*truncate) (struct vfs_inode *inode, u32 length);
    i32 (*lookup) (struct vfs_inode *inode, const char *name, struct vfs_inode **res);
    i32 (*create) (struct vfs_inode *dir, const i8 *name, i32 mode, struct vfs_inode **res);
    i32 (*symlink) (struct vfs_inode *dir, const i8 *name, const i8 *newname);
    i32 (*readlink) (struct vfs_inode *inode, i8 *buf, i32 bufsiz);

    struct vfs_inode *(*followlink) (struct vfs_inode *inode, struct vfs_inode *base);
    i32 (*mount) (struct vfs_inode *dir, u32 dev, const char *basename);
};

struct file_ops {
    i32 (*open) (struct vfs_inode *inode, struct file *fp);
    i32 (*read) (struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
    i32 (*write) (struct vfs_inode *inode, struct file *fp, i8 *buf, i32 count);
    i32 (*readdir) (struct vfs_inode *inode, struct file *fp, struct dirent *dent);
};

struct ext2_inode;
struct ext2_super_block;

struct vfs_inode {
    u32 i_mode;
    u32 i_uid;
    u32 i_gid;
    u32 i_size;
    u32 i_atime;
    u32 i_ctime;
    u32 i_mtime;
    u32 i_links_count;
    u32 i_blocks;
    u32 i_flags;
    u32 i_dev;
    u32 i_rdev;
    u32 i_num;
    u32 i_count;
    u32 i_dirt;
    u32 i_pipe;
    struct proc *i_wait;
    bool i_mount;
    struct vfs_superblock *i_sb;
    struct vfs_inode_ops *i_ops;
    struct file_ops *i_f_ops;
    union {
        struct ext2_inode i_ext2;
        struct pipe_inode i_pipe;
    } u;
};

struct fs_ops {
    void (*read_inode)(struct vfs_inode *vnode);
    void (*write_inode)(struct vfs_inode *vnode);
    void (*free_inode)(struct vfs_inode *vnode);
};

struct vfs_superblock {
    u32 s_dev;
    u32 s_block_size; // s_block_size = 1024 << p->s_log_block_size;
    struct vfs_inode *s_mounted;
    struct vfs_inode *s_root;
    struct fs_ops *fs_ops;
    union {
        struct ext2_super_block ext2_sb;
    } u;
};

struct filesystem {
    char *name;
    i32 (*read_super)(struct vfs_superblock *vsb);
};


struct file {
    u16 f_mode;
    u16 f_flags;
    u16 f_count;
    struct vfs_inode *f_inode;
    struct file_ops *f_ops; 
    i32 f_pos;
};

void mount_root(void);
struct vfs_superblock *get_vfs_super(u32 dev);
i32 vfs_do_mount(u32 dev, struct vfs_inode *dir);

struct vfs_inode *vfs_iget(u16 dev, u32 nr);
void vfs_iput(struct vfs_inode *inode);
i32 vfs_dirnamei(const i8 *pathname, struct vfs_inode *base, const i8 **res_basename, struct vfs_inode **res_inode);
i32 vfs_namei(const i8 *pathname, struct vfs_inode *base, i32 follow_links, struct vfs_inode **res);
i32 vfs_open_namei(i8 *pathname, i32 oflags, i32 mode, struct vfs_inode **res_inode);
i32 check_permission(struct vfs_inode *inode, i32 mask);
struct vfs_inode *get_empty_inode(void);

void sync_inodes(void);

#endif
