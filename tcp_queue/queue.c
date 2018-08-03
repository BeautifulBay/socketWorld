#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "list.h"
#include "queue.h"

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
		        (type *)( (char *)__mptr - offsetof(type,member) );})

void queue_init(struct queue_head *queue, void *data)
{
	list_init(&queue->sd_list);
	list_init(&queue->queue_list);
	queue->data = data;
}

void enqueue(struct list_head *head, struct queue_head *list)
{
	list_add_tail(head, &list->queue_list);
}

struct queue_head* dequeue(struct list_head *head)
{
	struct queue_head *list;
	list = container_of(head->next, struct queue_head, queue_list);
	list_delete(head, head->next);
	return list;
}

bool queue_is_empty(struct queue_head *head)
{
	return (head->queue_list.next == head->queue_list.prev);
}

