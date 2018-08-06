#ifndef __QUEUE_H_
#define __QUEUE_H_
#include <netinet/in.h>

struct queue_head {
#define BUF_LEN 64
	struct list_head sd_list;
	struct list_head queue_list;
	struct sockaddr_in client_addr;
	char from_data[BUF_LEN];
	int client_sd;
	void *data;
	int nread;
};

void queue_init(struct queue_head *queue, void *data);
void enqueue(struct list_head *head, struct queue_head *list);
struct queue_head* dequeue(struct list_head *head);

#endif
