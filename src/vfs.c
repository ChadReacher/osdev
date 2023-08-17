#include "vfs.h"
#include "list.h"
#include "heap.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"
#include "memory.h"
#include "list.h"


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

	vfs_node_t *node = malloc(sizeof(vfs_node_t));
	strcpy(node->name, "/");
	node->mask = 0; 
	node->uid = 0;
	node->gid = 0;
	node->inode = 0;
	node->length = 0;
	node->read = NULL;
	node->create = NULL;
	node->unlink = NULL;
	node->write = NULL;
	node->open = NULL; 
	node->close = NULL; 
	node->readdir = NULL;
	node->finddir = NULL;
	node->ptr = NULL;
	node->flags = 0;

	tree_node_t *tree_node_root = malloc(sizeof(tree_node_t));
	tree_node_root->val = node;
	tree_node_root->children = list_create();
	tree_node_root->parent = tree_node_root;

	vfs_root = node;
	vfs_tree->root = tree_node_root;
	vfs_tree->sz = 1;

	DEBUG("%s", "Virtual file system has been successfully initialized\r\n");
}

// Given a full absolute path, returns a corresponding vfs node in VFS tree
vfs_node_t *vfs_get_node(i8 *path) {
	i8 *name, *path_dup;
	list_t *children;
	list_node_t *child;

	if (!vfs_tree || !vfs_tree->root || !vfs_tree->sz 
		|| path == NULL || path[0] != '/') {
		return NULL;
	}

	if (!vfs_tree->root->val && !vfs_root) {
		return NULL;
	}

	if (strlen(path) == 1 && path[0] == '/') {
		return vfs_tree->root->val;
	}

	path_dup = strdup(path);
	i8 *saved_pathdup = path_dup;
	path_dup = path_dup + 1;
	tree_node_t *current_tree_node = vfs_tree->root;	
	bool found = false;

	while ((name = strsep(&path_dup, "/")) != NULL) {
		found = false;
		children = current_tree_node->children;
		for(child = children->head; child; child = child->next) {
			tree_node_t *tree_node = (tree_node_t *)child->val;
			vfs_node_t *vnode = (vfs_node_t*)tree_node->val;
			if (strcmp(vnode->name, name) == 0) {
				found = true;
				current_tree_node = (tree_node_t *)child->val;
				break;
			}
		}
		if (!found) {
			break;
		}
	}

	if (found) {
		free(saved_pathdup);
		return current_tree_node->val;
	}

	// If we haven't found, then try to use physical fs's
	// functions to find the node
	vfs_node_t *node_ptr = current_tree_node->val;

	node_ptr = vfs_finddir(node_ptr, name);
	if (!node_ptr) {
		free(saved_pathdup);
		return NULL;
	}
	while ((name = strsep(&path_dup, "/")) != NULL) {
		node_ptr = vfs_finddir(node_ptr, name);
		if (!node_ptr) {
			free(saved_pathdup);
			return NULL;
		}
	}

	free(saved_pathdup);
	return node_ptr;
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

void vfs_create(i8 *name, u16 permission) {
	if (!name) {
		return;
	} else if (name[0] != '/') {
		return;
	} else if (strlen(name) == 1 && name[0] == '/') {
		return;
	}
	// Get parent directory vfs node
	i32 i = strlen(name) - 1;
	i8 *dirname = strdup(name);
	i8 *saved_dirname = dirname;
	i8 *parent_path = "/";
	while (i >= 0) {
		if (dirname[i] == '/') {
			if (i != 0) {
				dirname[i] = '\0';
				parent_path = dirname;
			}
			dirname = dirname + i + 1;
			break;
		}
		--i;
	}

	vfs_node_t *parent_node = vfs_get_node(parent_path);
	if (!parent_node) {
		free(saved_dirname);
		return;
	}

	if ((parent_node->flags & FS_DIRECTORY) == FS_DIRECTORY && parent_node->create) {
		parent_node->create(parent_node, dirname, permission);
	}
	free(saved_dirname);
}

void vfs_mkdir(i8 *name, u16 permission) {
	// Get parent directory vfs node
	i32 i = strlen(name);
	i8 *dirname = strdup(name);
	i8 *saved_dirname = dirname;
	i8 *parent_path = "/";
	while (i >= 0) {
		if (dirname[i] == '/') {
			if (i != 0) {
				dirname[i] = '\0';
				parent_path = dirname;
			}
			dirname = dirname + i + 1;
			break;
		}
		--i;
	}

	DEBUG("Want to create a file - %s\r\n", name);
	DEBUG("Dirname - %s\r\n", dirname);
	DEBUG("Parent path - %s\r\n", parent_path);
	vfs_node_t *parent_node = vfs_get_node(parent_path);
	if (!parent_node) {
		free(saved_dirname);
		return;
	}

	if ((parent_node->flags & FS_DIRECTORY) == FS_DIRECTORY && parent_node->mkdir) {
		parent_node->mkdir(parent_node, dirname, permission);
	}
	free(saved_dirname);
}

dirent *vfs_readdir(vfs_node_t *vfs_node, u32 index) {
	if (vfs_node && ((vfs_node->flags & FS_DIRECTORY) == FS_DIRECTORY) && vfs_node->readdir) {
		dirent *de = vfs_node->readdir(vfs_node, index);
		return de;
	}
	return (dirent *)NULL;
}

vfs_node_t *vfs_finddir(vfs_node_t *node, i8 *name) {
	if (node && (node->flags & FS_DIRECTORY) && node->finddir) {
		return node->finddir(node, name);
	}
	return (vfs_node_t *)NULL;
}

void vfs_unlink(i8 *name) {
	// Get parent directory vfs node
	i32 i = strlen(name);
	i8 *dirname = strdup(name);
	i8 *saved_dirname = dirname;
	i8 *parent_path = "/";
	while (i >= 0) {
		if (dirname[i] == '/') {
			if (i != 0) {
				dirname[i] = '\0';
				parent_path = dirname;
			}
			dirname = dirname + i + 1;
			break;
		}
		--i;
	}

	DEBUG("Want to delete a file - %s\r\n", name);
	DEBUG("Dirname - %s\r\n", dirname);
	DEBUG("Parent path - %s\r\n", parent_path);
	vfs_node_t *parent_node = vfs_get_node(parent_path);
	if (!parent_node) {
		free(saved_dirname);
		return;
	}

	if ((parent_node->flags & FS_DIRECTORY) == FS_DIRECTORY && parent_node->unlink) {
		parent_node->unlink(parent_node, dirname);
	}
	free(saved_dirname);
}

void vfs_mount(i8 *path, vfs_node_t *vfs_node_to_mount) {
	i8 *name, *path_dup;
	list_t *children;
	list_node_t *child;

	if (!vfs_tree || !vfs_tree->root || !vfs_tree->sz 
		|| path == NULL || path[0] != '/' || !vfs_node_to_mount) {
		return;
	}

	// Are we trying to mount the root?
	if (strlen(path) == 1 && path[0] == '/') {
		if (((vfs_node_t *)(vfs_tree->root->val))->length && vfs_root) {
			DEBUG("%s", "Vfs tree root is already mounted\r\n");
			return;
		}
		vfs_root = vfs_node_to_mount;
		vfs_tree->root->val = vfs_node_to_mount;
		return;
	}

	if (!vfs_tree->root->val && !vfs_root) {
		DEBUG("Cannot mount the path(%s) because the root is not mounted\r\n", path);
		return;
	}

	path_dup = strdup(path);
	i8 *saved_pathdup = path_dup;
	path_dup = path_dup + 1;
	tree_node_t *current_tree_node = vfs_tree->root;	
	bool found = false;

	while ((name = strsep(&path_dup, "/")) != NULL) {
		found = false;
		children = current_tree_node->children;
		for(child = children->head; child; child = child->next) {
			tree_node_t *vvnode = (tree_node_t *)child->val;
			vfs_node_t *vnode = (vfs_node_t*)vvnode->val;
			if (strcmp(vnode->name, name) == 0) {
				found = true;
				current_tree_node = (tree_node_t *)child->val;
				break;
			}
		}
		if (!found) {
			if (path_dup) {
				vfs_node_t *intermediary_node = malloc(sizeof(vfs_node_t));
				strcpy(intermediary_node->name, name);
				intermediary_node->mask = 0; 
				intermediary_node->uid = 0;
				intermediary_node->gid = 0;
				intermediary_node->inode = 0;
				intermediary_node->length = 0; 
				intermediary_node->read = NULL;
				intermediary_node->write = NULL;
				intermediary_node->create = NULL;
				intermediary_node->unlink = NULL;
				intermediary_node->open = NULL;
				intermediary_node->close = NULL;
				intermediary_node->readdir = NULL;
				intermediary_node->finddir = NULL;
				intermediary_node->ptr = NULL;
				intermediary_node->flags = FS_DIRECTORY;
				current_tree_node = generic_tree_insert_at(vfs_tree, current_tree_node, intermediary_node);
			} else {
				generic_tree_insert_at(vfs_tree, current_tree_node, vfs_node_to_mount);
				break;
			}
		}
	}
	free(saved_pathdup);
	
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
	if (!vfs_tree || !vfs_tree->root->val) {
		return;
	}
	DEBUG("Size of VFS tree: %x\r\n", vfs_tree->sz);
	tree_node_t *root = vfs_tree->root;
	vfs_node_t *vfs_node = (vfs_node_t *)root->val;
	DEBUG("'%s'\r\n", vfs_node->name);
	vfs_print_node(vfs_tree->root);
}

// Caller should free the memory
i8 *canonilize_path(i8 *full_path) {
	if (!full_path || full_path[0] != '/') {
		return NULL;
	}
	list_t *tokens_list = list_create();
	i8 *str = strdup(full_path);
	i8 *rest = str;
	i8 *token;
	while ((token = strsep(&rest, "/")) != NULL) {
		if (strcmp(token, ".") == 0) {
			continue;
		} else if (strcmp(token, "..") == 0) {
			if (tokens_list->sz > 0) {
				free(list_remove_back(tokens_list));
			}
		} else {
			list_insert_back(tokens_list, strdup(token));
		}
	}

	if (tokens_list->sz == 0) {
		i8 *canonilized_path = malloc(2);
		canonilized_path[0] = '/';
		canonilized_path[1] = '\0';

		list_destroy(tokens_list);
		free(str);
		return canonilized_path;
	}

	i8 *canonilized_path = malloc(256);
	memset(canonilized_path, 0, 256);

	// Remove and free the ""(empty) string
	// i.e the result of the first separating before "/"
	if (tokens_list->sz > 1) {
		free(list_remove_front(tokens_list));
	}

	while (tokens_list->sz) {
		list_node_t *node = list_remove_front(tokens_list);
		i8 *temp = (i8 *)node->val;
		strcat(canonilized_path, "/");
		strcat(canonilized_path, temp);
		free(node);
	}

	list_destroy(tokens_list);
	free(str);
	return canonilized_path;
}
