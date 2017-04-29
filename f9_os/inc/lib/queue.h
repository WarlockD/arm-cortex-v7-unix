/* Copyright (c) 2013 The F9 Microkernel Project. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef LIB_QUEUE_H_
#define LIB_QUEUE_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
	QUEUE_OK=0,
	QUEUE_OVERFLOW,
	QUEUE_EMPTY,
} queue_status_t;
/**
 * Queue control block
 */
typedef struct  {
	uint8_t *data;		/*!< pointer to the starting position of buffer */
	uint32_t top;		/*!< queue top */
	uint32_t end;		/*!< queue end */
	size_t size;		/*!< the size of the allocated queue */
}queue_t;

static inline uint32_t queue_length(queue_t *queue)
{
	return (queue->end >= queue->top) ?
	       (queue->end - queue->top) :
	       (queue->size + queue->top - queue->end);
}

static inline queue_status_t queue_init(queue_t *queue, uint8_t *addr, size_t size)
{
	queue->top = 0;
	queue->end = 0;
	queue->size = size;
	queue->data = addr;

	return QUEUE_OK;
}

static inline queue_status_t queue_push(queue_t *queue, uint8_t element)
{
	if (queue_length(queue) == queue->size)
		return QUEUE_OVERFLOW;

	++queue->end;

	if (queue->end == (queue->size - 1))
		queue->end = 0;

	queue->data[queue->end] = element;

	return QUEUE_OK;
}

static inline bool queue_is_empty(queue_t *queue)
{
	return queue_length(queue) == 0;
}

static inline queue_status_t queue_pop(queue_t *queue, uint8_t *element)
{
	if (queue_length(queue) == 0)
		return QUEUE_EMPTY;

	++queue->top;

	if (queue->top == (queue->size - 1))
		queue->top = 0;

	*element = queue->data[queue->top];

	return QUEUE_OK;
}


#endif /* LIB_QUEUE_H_ */
