#include <stdio.h>
#include "list.h"

void list_init(struct list_head *list)
{
	list->prev = list;
	list->next = list;
}

void list_add_tail(struct list_head *head, struct list_head *list)
{
	head->prev->next = list;
	list->prev = head->prev;
	list->next = head;
	head->prev = list;
}

void list_delete(struct list_head *head, struct list_head *list){
	list->prev->next = head;
	head->prev = list->prev;
	list->prev = NULL;
	list->next = NULL;
}

void list_delete_self(struct list_head *self)
{
	if (self->prev == NULL || self->next == NULL)
		return;
	self->prev->next = self->next;
	self->next->prev = self->prev;
	self->prev = NULL;
	self->next = NULL;
}

bool list_is_empty(struct list_head *list)
{
	return (list->prev == list && list->next == list) || (list->prev == NULL || list->next == NULL);
}
