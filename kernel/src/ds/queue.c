#include <queue.h>
#include <stdlib.h>
#include <heap.h>
#include <string.h>
#include <panic.h>

queue_t *queue_new() {
	queue_t *self = malloc(sizeof(queue_t));
	memset(self, 0, sizeof(queue_t));
	return self;
}

void queue_delete(queue_t *self) {
	u32 i;
	for (i = 0; i < self->len; ++i) {
		queue_dequeue(self);
	}
	free(self);
}

void queue_enqueue(queue_t *self, void *value) {
	queue_node_t *node;
	if (self == NULL) {
		return;
	}
	node = malloc(sizeof(queue_node_t));
	memset(node, 0, sizeof(queue_node_t));
	node->value = value;
	if (!self->len) { 
		self->head = self->tail = node;
	} else {
		self->tail->next = node;
		node->prev = self->tail;
		self->tail = node;
	}
	++self->len;
}

void *queue_dequeue(queue_t *self) {
	queue_node_t *old;
	void *value;

	if (self == NULL || self->len == 0) {
		return NULL;
	}
	--self->len;
	old = self->head;
	self->head = self->head->next;
	if (self->head != NULL) {
		self->head->prev = NULL;
	} else {
		self->tail = NULL;
	}
	value = old->value;
	free(old);

	return value;
}

void queue_remove(queue_t *self, queue_node_t *node) {
	if (node->prev == NULL) {
		node->next->prev = NULL;
		self->head = node->next;
	} else {
		node->prev->next = node->next;
		if (node->next) {
			node->next->prev = node->prev;
		} else {
			self->tail = node->prev;
		}
	}
	free(node);
	--self->len;
}

