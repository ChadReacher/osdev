#ifndef LIST_H
#define LIST_H

#include "types.h"

typedef struct list_node {
	struct list_node *next;
	struct list_node *prev;
	void *val;
} list_node_t;

typedef struct list {
	list_node_t *head;
	list_node_t *tail;
	u32 sz;
} list_t;

list_t *list_create();
list_node_t *list_insert_front(list_t *list, void *val);
void list_insert_back(list_t *list, void *val);
list_node_t *list_remove_front(list_t *list);
list_node_t *list_remove_back(list_t *list);
list_node_t *list_remove_node(list_t *list, list_node_t *node);
void list_destroy(list_t *list);

#endif
