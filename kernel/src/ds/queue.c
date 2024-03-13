#include <queue.h>
#include <stdlib.h>
#include <heap.h>
#include <string.h>
#include <debug.h>

queue_t *queue_new() {
	//DEBUG("malloc(sizeof(queue_t)) - %d\r\n", sizeof(queue_t));
	queue_t *self = malloc(sizeof(queue_t));
	memset(self, 0, sizeof(queue_t));
	return self;
}

void queue_delete(queue_t *self) {
	for (u32 i = 0; i < self->len; ++i) {
		queue_dequeue(self);
	}
	//DEBUG("free(self);\r\n");
	free(self);
}

void queue_enqueue(queue_t *self, void *value) {
	if (self == NULL) {
		return;
	}
	//DEBUG("malloc(sizeof(queue_t)) - %d\r\n", sizeof(queue_t));
	queue_node_t *node = malloc(sizeof(queue_node_t));
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
	if (self == NULL || self->len == 0) {
		return NULL;
	}
	--self->len;
	queue_node_t *old = self->head;
	self->head = self->head->next;
	if (self->head != NULL) {
		self->head->prev = NULL;
	} else {
		self->tail = NULL;
	}
	void *value = old->value;
	//DEBUG("free(old);\r\n");
	free(old);

	return value;
}
