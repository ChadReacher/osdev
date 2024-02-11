#ifndef QUEUE_H
#define QUEUE_H

#include "types.h"

typedef struct queue_node {
	void *value;
	struct queue_node *next;
	struct queue_node *prev;
} queue_node_t;

typedef struct {
	u32 len;
	queue_node_t *head;
	queue_node_t *tail;
} queue_t;

queue_t *queue_new(); 
void queue_delete(queue_t *self);
void queue_enqueue(queue_t *self, void *value);
void *queue_dequeue(queue_t *self);

#endif
