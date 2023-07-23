#include "generic_tree.h"
#include "heap.h"
#include "debug.h"

tree_t *generic_tree_create() {
	tree_t *tree = malloc(sizeof(tree_t));
	tree->sz = 0;
	tree->root = NULL;
	return tree;
}

static void generic_tree_destroy_node(tree_node_t *node) {
	list_t *children = node->children;
	if (!children) {
		free(node);
		return;
	}

	for (list_node_t *child = children->head; child != NULL; child = child->next) {
		generic_tree_destroy_node(child->val);
	}
}

tree_node_t *generic_tree_insert_at(tree_t *tree, tree_node_t *parent, void *value) {
	if (!tree || !parent) {
		return NULL;
	}
	tree_node_t *new_tree_node = malloc(sizeof(tree_node_t));
	new_tree_node->val = value;
	new_tree_node->children = list_create();
	new_tree_node->parent = parent;

	list_insert_back(parent->children, new_tree_node);
	++tree->sz;
	return new_tree_node;
}

static void generic_tree_print_node(tree_node_t *node) {
	if (!node) {
		return;
	}
	DEBUG("Tree node(%p), value = %d\r\n", node, node->val);
	if (node->children->sz == 0) {
		return;
	}
	list_t *children = node->children;
	for (list_node_t *child = children->head; child != NULL; child = child->next) {
		DEBUG("Children(of %d): \r\n", node->val);
		generic_tree_print_node(child->val);
	}
}

void generic_tree_print(tree_t *tree) {
	if (!tree || !tree->root) {
		return;
	}
	generic_tree_print_node(tree->root);
	DEBUG("%s", "\r\n");
}

void generic_tree_destroy(tree_t *tree) {
	if (!tree || !tree->root) {
		return;
	}
	generic_tree_destroy_node(tree->root);
}
