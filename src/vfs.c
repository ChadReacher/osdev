#include "vfs.h"
#include "list.h"
#include "heap.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"


// TODO: Should we strip '/' from the start and the end of path? 
// For example: /home/games/host/ should become
// home/games/host.
// If we want to do it, then we should implement string
// operation to strip a string from a character.

// n-ary virtual file system tree
tree_t *vfs_tree = NULL;
// pointer to the root of vfs tree
vfs_node_t *vfs_root = NULL;

void vfs_init() {
	vfs_tree = generic_tree_create();
	vfs_tree->sz = 1;

	// Should we create an empty directory(w/o fs) at the root?
	// Will something be mounted at root?

	vfs_node_t *vfs_node_root = malloc(sizeof(vfs_node_t));
	strcpy(vfs_node_root->name, "/");
	vfs_node_root->mask = vfs_node_root->uid = vfs_node_root->gid = vfs_node_root->inode = vfs_node_root->length = vfs_node_root->impl = 0;
	vfs_node_root->read = NULL;
	vfs_node_root->write = NULL;
	vfs_node_root->open = NULL; 
	vfs_node_root->close = NULL; 
	vfs_node_root->readdir = NULL;
	vfs_node_root->finddir = NULL;
	vfs_node_root->ptr = NULL;

	vfs_node_root->flags = 0; // ?

	tree_node_t *tree_node_root = malloc(sizeof(tree_node_t));
	tree_node_root->val = vfs_node_root;
	tree_node_root->children = list_create();
	tree_node_root->parent = tree_node_root;

	vfs_root = vfs_node_root;
	vfs_tree->root = tree_node_root;

	DEBUG("%s", "Virtual file system has been successfully initialized\r\n");
}

u32 vfs_read(vfs_node_t *node, u32 offset, u32 size, i8 *buf) {
	if (node && node->read) {
		u32 ret = node->read(node, offset, size, buf);
		return ret;
	}
	return 0;
}

u32 vfs_write(vfs_node_t *node, u32 offset, u32 size, i8 *buf) {
	if (node && node->write) {
		u32 ret = node->write(node, offset, size, buf);
		return ret;
	}
	return 0;
}

void vfs_open(vfs_node_t *node, u32 flags) {
	if (!node) {
		return;
	}
	if (node->open) {
		node->open(node, flags);
	}
}

void vfs_close(vfs_node_t *node) {
	if (!node) {
		return;
	}
	if (node->close) {
		node->close(node);
	}
}

struct dirent *vfs_readdir(tree_node_t *tree_node, u32 index) {
	if (!tree_node) {
		return NULL;
	}
	vfs_node_t *vfs_node = (vfs_node_t *)tree_node->val;

	if (vfs_node && ((vfs_node->flags & FS_DIRECTORY) == FS_DIRECTORY) && vfs_node->readdir) {
		struct dirent *de;
		if (index == 0) {
			de = malloc(sizeof(struct dirent));
			strcpy(de->name, ".");
			de->inode = vfs_node->inode;
			return de;
		} else if (index == 1) {
			de = malloc(sizeof(struct dirent));
			strcpy(de->name, "..");

			vfs_node_t *parent_vfs_node = (vfs_node_t *)tree_node->parent->val;
			de->inode = parent_vfs_node->inode;
			return de;
		}

		bool node_is_mounpoint = (vfs_node->flags & FS_MOUNTPOINT) == FS_MOUNTPOINT;

		if (node_is_mounpoint && tree_node->children->sz > (index - 2)) {
			de = malloc(sizeof(struct dirent));
			list_node_t *vfs_child_list_node = tree_node->children->head;
			for (size_t i = 0; i < (index - 2) && vfs_child_list_node; ++i) {
				vfs_child_list_node = vfs_child_list_node->next;
			}
			vfs_node_t *vfs_node_child = (vfs_node_t *)vfs_child_list_node->val;

			de->inode = vfs_node_child->inode;
			strcpy(de->name, vfs_node_child->name);
			return de;
		}
		
		if (node_is_mounpoint) {
			index -= tree_node->children->sz;
		}
		de = vfs_node->readdir(vfs_node, index);

		return de;
	}
	return NULL;
}

vfs_node_t *vfs_finddir(vfs_node_t *node, i8 *name) {
	if (node && (node->flags & FS_DIRECTORY) && node->finddir) {
		return node->finddir(node, name);
	}
	return NULL;
}

void vfs_mount(i8 *path, vfs_node_t *vfs_node_to_mount) {
	i8 *name, *path_dup;
	list_t *children;
	list_node_t *child;

	if (!vfs_tree || !vfs_tree->root  || !vfs_tree->sz || path == NULL || path[0] != '/' || !vfs_node_to_mount) {
		return;
	}

	path_dup = strdup(path);
	path_dup = path_dup + 1;
	tree_node_t *current_tree_node = vfs_tree->root;	
	bool found = false;

	while ((name = strsep(&path_dup, "/")) != NULL) {
		found = false;
		DEBUG("Get from strsep - %s, %c, (%x)\r\n", name, *name, *name);
		children = current_tree_node->children;
		DEBUG("Children of '%s' are:\r\n", ((vfs_node_t*)current_tree_node->val)->name);
		for(child = children->head; child; child = child->next) {
			tree_node_t *vvnode = (tree_node_t *)child->val;
			vfs_node_t *vnode = (vfs_node_t*)vvnode->val;
			DEBUG("Vnode child name : %s\r\n", vnode->name);
			if (strcmp(vnode->name, name) == 0) {
				found = true;
				current_tree_node = (tree_node_t *)child->val;
				break;
			}
		}
		if (!found) {
			if (path_dup) {
				DEBUG("Cannot mount a vfs node, because the path(%s) is not present in the VFS tree\r\n", path);
			} else {
				generic_tree_insert_at(vfs_tree, current_tree_node, vfs_node_to_mount);
			}
			break;
		}
	}
	free(path_dup);
	
	// Trying to mount to an already existing node
	if (found) {
		DEBUG("The node at the path %s is already mounted.\r\n", path);
	}
}

void vfs_print_node(tree_node_t *node) {
	if (!node) {
		return;
	}
	list_t *children_list = node->children;
	list_node_t *children_head = children_list->head;
	list_node_t *child_list_node = children_head;
	while (child_list_node) {
		tree_node_t *list_node_value = (tree_node_t *)child_list_node->val;
		vfs_node_t *vfs_node = (vfs_node_t *)list_node_value->val;
		DEBUG("'%s'\r\n", vfs_node->name);
		child_list_node = child_list_node->next;
	}
	DEBUG("%s", "\r\n");

	child_list_node = children_head;	
	while (child_list_node) {
		tree_node_t *list_node_value = (tree_node_t *)child_list_node->val;
		vfs_print_node(list_node_value);
		child_list_node = child_list_node->next;
	}
}

void vfs_print() {
	if (!vfs_tree) {
		return;
	}
	DEBUG("Size of VFS tree: %x\r\n", vfs_tree->sz);
	tree_node_t *root = vfs_tree->root;
	vfs_node_t *vfs_node = (vfs_node_t *)root->val;
	DEBUG("'%s'\r\n", vfs_node->name);
	vfs_print_node(vfs_tree->root);
}