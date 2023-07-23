#ifndef GENERIC_TREE_H
#define GENERIC_TREE_H

#include "types.h"
#include "list.h"

typedef struct tree_node {
	void *val;
	list_t *children;
	struct tree_node *parent;
} tree_node_t;

typedef struct {
	u32 sz;
	tree_node_t *root;
} tree_t;

tree_t *generic_tree_create();
void generic_tree_destroy(tree_t *tree);
tree_node_t *generic_tree_insert_at(tree_t *tree, tree_node_t *parent, void *value);
void generic_tree_print(tree_t *tree);

#endif
