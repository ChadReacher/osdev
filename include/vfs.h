#ifndef VFS_H
#define VFS_H

#include "types.h"
#include "generic_tree.h"

#define FS_FILE			0x01
#define FS_DIRECTORY	0x02
#define FS_CHARDEVICE	0x03
#define FS_BLOCKDEVICE	0x04
#define FS_PIPE			0x05
#define FS_SYMLINK		0x06
#define FS_MOUNTPOINT	0x08

typedef struct {
	char name[256];
	u32 inode;
} dirent;

struct vfs_node;

typedef u32 (*read_callback)(struct vfs_node *node, u32 offset, u32 size, i8* buf);
typedef u32 (*write_callback)(struct vfs_node *node, u32 offset, u32 size, i8* buf);
typedef u32 (*open_callback)(struct vfs_node *node, u32 flags);
typedef u32 (*close_callback)(struct vfs_node *node);
typedef void (*create_callback)(struct vfs_node *node, i8 *name, u16 permission);
typedef void (*mkdir_callback)(struct vfs_node *node, i8 *name, u16 permission);
typedef dirent * (*readdir_callback)(struct vfs_node *node, u32 index);
typedef struct vfs_node * (*finddir_callback)(struct vfs_node* node, i8 *name);
typedef i32 (*unlink_callback)(struct vfs_node *node, i8 *name);
typedef void (*trunc_callback)(struct vfs_node *node);

typedef struct vfs_node {
	i8 name[256];				// The filename
	u32 permission_mask;		// The permissions mask
	u32 flags;					// include node type(file, directory, char device, block device, pipe, symlink, mountpoint)
	u32 uid;					// The owning user
	u32 gid;					// The group id
	u32 inode;					// This is a device-specific - provides a way for filesystem to identify files
	u32 length;					// Size of file, in bytes
	u32 atime;
	u32 mtime;
	u32 ctime;

	read_callback read;
	write_callback write;
	open_callback open;
	close_callback close;
	create_callback create;
	mkdir_callback mkdir;
	unlink_callback unlink;
	trunc_callback trunc;
	readdir_callback readdir;	// Return n'th child of a directory 
	finddir_callback finddir;   // Try to find a child in a directory
	void *device;				// Possible char or block device
} vfs_node_t;

#define FDS_NUM 32

typedef struct {
	vfs_node_t *vfs_node;
	u32 offset;
	i32 flags;
	bool used;
} file;

void vfs_init();
vfs_node_t *vfs_get_node(i8 *path);
u32 vfs_read(vfs_node_t *node, u32 offset, u32 size, i8 *buf);
u32 vfs_write(vfs_node_t *node, u32 offset, u32 size, i8 *buf);
void vfs_open(vfs_node_t *node, u32 flags);
void vfs_close(vfs_node_t *node);
void vfs_create(i8 *name, u16 permission);
void vfs_mkdir(i8 *name, u16 permission);
dirent *vfs_readdir(vfs_node_t *vfs_node, u32 index);
vfs_node_t *vfs_finddir(vfs_node_t *node, i8 *name);
i32 vfs_unlink(i8 *name);
void vfs_trunc(vfs_node_t *vfs_node);

void vfs_mount(i8 *path, vfs_node_t *vfs_node);
void vfs_print_node(tree_node_t *node);
void vfs_print();

i8 *canonilize_path(i8 *full_path);

#endif
