#include <list.h>
#include <heap.h>

list_t *list_create() {
	list_t *list = malloc(sizeof(list_t));
	list->head = NULL;
	list->tail = NULL;
	list->sz = 0;
	return list;
}

list_node_t *list_insert_front(list_t *list, void *val) {
	if (!list) {
		return NULL;
	}
	list_node_t *node = malloc(sizeof(list_node_t));

	if (!list->head) {
		node->next = NULL;
		node->prev = NULL;
		node->val = val;
		list->head = node;
		list->tail = node;
		++list->sz;
		return node;
	}

	list->head->prev = node;
	node->next = list->head;
	node->prev = NULL;
	node->val = val;

	list->head = node;
	++list->sz;
	return node;
}

list_node_t *list_insert_back(list_t *list, void *val) {
	if (!list) {
		return NULL;
	}

	list_node_t *node = malloc(sizeof(list_node_t));
	if (!list->tail) {
		node->next = NULL;
		node->prev = NULL;
		node->val = val;
		list->tail = node;
		list->head = node;
		++list->sz;
		return node;
	}

	list->tail->next = node;
	node->prev = list->tail;
	node->next = NULL;
	node->val = val;

	list->tail = node;
	++list->sz;
	return node;
}

// Caller should free the memory
list_node_t *list_remove_front(list_t *list) {
	if (!list || !list->head) {
		return NULL;
	}
	if (list->sz == 1) {
		list_node_t *save = list->head;
		list->head = NULL;
		list->tail = NULL;
		list->sz = 0;
		return save;
	}
	list_node_t *save = list->head;
	list->head = list->head->next;
	list->head->prev = NULL;
	--list->sz;
	return save;
}

// Caller should free the memory
list_node_t *list_remove_back(list_t *list) {
	if (!list || !list->tail) {
		return NULL;
	}
	if (list->sz == 1) {
		list_node_t *save = list->tail;
		list->head = NULL;
		list->tail = NULL;
		list->sz = 0;
		return save;
	}
	list_node_t *save = list->tail;
	list->tail = list->tail->prev;
	list->tail->next = NULL;
	--list->sz;
	return save;
}

// Caller should free the memory
list_node_t *list_remove_node(list_t *list, list_node_t *node) {
	if (!list || !list->head) {
		return NULL;
	}

	if (list->head == node) {
		return list_remove_front(list);	
	} else if (list->tail == node) {
		return list_remove_back(list);
	} else {
		// 1 - [2] - 3
		node->next->prev = node->prev;
		node->prev->next = node->next;
		--list->sz;
		return node;
	}
}

void list_destroy(list_t *list) {
	list_node_t *node = list->head;
	while (node) {
		list_node_t *save = node;
		node = node->next;
		free(save);
	}
	free(list);
}
