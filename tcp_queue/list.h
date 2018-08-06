#ifndef __LIST_H_
#define __LIST_H_
#include <stdbool.h>

struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

void list_init(struct list_head *list);
void list_add_tail(struct list_head *head, struct list_head *list);
void list_delete_self(struct list_head *self);
bool list_is_empty(struct list_head *list);

#endif
