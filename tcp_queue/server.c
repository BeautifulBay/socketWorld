#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include "list.h"
#include "queue.h"

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({ \
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
		(type *)( (char *)__mptr - offsetof(type,member) );})

struct server_data {
#define SERVER_PORT 8888
#define SERVER_ADDR "127.0.0.1"
#define BUF_LEN 	64
#define CLIENT_NUM 	64
#define EPOLL_SIZE_MAX 1024
	int server_sd;
	struct sockaddr_in server_addr;
	char to_data[BUF_LEN];
	char from_data[BUF_LEN];
	int count;
	int len;
	struct list_head sd_head;
	struct list_head queue_head;
	int epoll_fd;
	struct epoll_event ev;
	struct epoll_event polledevents[EPOLL_SIZE_MAX];
	int npolledevents;
	sem_t queue_sem;
	pthread_mutex_t queue_mutex;
};

int tcp_server_init(struct server_data *test)
{
	int ret;
	/* create socket fd */
	test->server_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (test->server_sd == -1) {
		printf("socket server_sd error!\n");
		return test->server_sd;
	}

	/* init */
	test->len = sizeof(struct sockaddr_in);
	list_init(&test->sd_head);		//list for all client
	list_init(&test->queue_head);	//list for client that have message to handle
	sem_init(&test->queue_sem, 0, 0);
	pthread_mutex_init(&test->queue_mutex, NULL);

	/* config server addr */
	memset(&test->server_addr, 0 , sizeof(struct sockaddr_in));
	test->server_addr.sin_family = AF_INET;
	test->server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	test->server_addr.sin_port = htons(SERVER_PORT);

	/* bind socket fd to server addr */
	ret = bind(test->server_sd, (struct sockaddr *)&test->server_addr, test->len);
	if (ret == -1) {
		printf("server bind %d error!", test->server_addr.sin_addr.s_addr);
		close(test->server_sd);
		return ret;
	}

	/* config server sd as listen sd */
	ret = listen(test->server_sd, CLIENT_NUM);
	if (ret == -1) {
		printf("server listen error!\n");
		close(test->server_sd);
		return ret;
	}

	/* create epoll fd */
	test->epoll_fd = epoll_create(EPOLL_SIZE_MAX);
	if (test->epoll_fd == -1) {
		printf("epoll_create error!\n");
		close(test->server_sd);
		return test->epoll_fd;
	}

	/* add listen sd to epoll */
	test->ev.events = EPOLLIN | EPOLLET;
	test->ev.data.fd = test->server_sd;
	epoll_ctl(test->epoll_fd, EPOLL_CTL_ADD, test->server_sd, &test->ev);

	/* add stdin to epoll */
	test->ev.events = EPOLLIN | EPOLLET;
	test->ev.data.fd = 0;
	epoll_ctl(test->epoll_fd, EPOLL_CTL_ADD, 0, &test->ev);
}

void signal_catchfunc(int number)
{
	pthread_exit(NULL);
}

void *handle_client_queue_thread(void *data)
{
	struct queue_head *queue = NULL;
	struct server_data *test = (struct server_data*)data;
	int ret;

	signal(SIGUSR1, signal_catchfunc);

	while(1) {
		sem_wait(&test->queue_sem);
		if (list_is_empty(&test->queue_head))
			continue;

		pthread_mutex_lock(&test->queue_mutex);
		/* dequeue */
		queue = dequeue(&test->queue_head);

		/* send data to client */
		memset(test->to_data, 0, BUF_LEN);
		snprintf(test->to_data, BUF_LEN, "Good job!");
		ret = write(queue->client_sd, test->to_data, strlen(test->to_data));
		if (ret == -1) {
			printf("write to a client fd %d error!\n", queue->client_sd);
		}
		pthread_mutex_unlock(&test->queue_mutex);
	}
}

int kill_thread(pthread_t tid, int signum)
{
	if (tid > 0) {
		int ret = pthread_kill(tid, 0);
		if (ret == ESRCH)
		    printf("No thread with the ID: %d\n", (unsigned int) tid);
		else if(ret == EINVAL)
		    printf("An invalid signal was specified\n");
		else {
		    pthread_kill(tid, signum);
		    printf("thread(thread id=%lu) be killed\n", tid);
		}
    }
	return 0;
}

void print_list(struct list_head *head)
{
	struct list_head *temp = head;
	/* loop to print list for debugging */
	while (temp->next != head)
	{
		struct queue_head *list = container_of(temp->next, struct queue_head, sd_list);
		printf("fd = %d\n", list->client_sd);
		temp = temp->next;
	}
}

int tcp_handle_client_data(struct server_data *test)
{
	int ret;
	int i;
	int nread;
	int flag = 0;
	pthread_t queue_id;

	/* create thread to handle message from client */
	ret = pthread_create(&queue_id, NULL, handle_client_queue_thread, (void*)test);
	if (ret != 0) {
		printf("pthread_create error!\n");
		return -1;
	}

	/* listen to all */
	while(1) {
		/* server quit flag */
		if (flag) {
			break;
		}

		test->npolledevents = epoll_wait(test->epoll_fd, test->polledevents, EPOLL_SIZE_MAX, -1);
		if (test->npolledevents <= 0) {
			continue;
		}

		for (i = 0; i < test->npolledevents; i++) {
			if (test->polledevents[i].data.fd == test->server_sd) {
				/* client sd connect */
				struct queue_head *queue = (struct queue_head *)malloc(sizeof(struct queue_head));
				queue->client_sd = accept(test->server_sd, (struct sockaddr*)&queue->client_addr, &test->len);
				if (queue->client_sd == -1) {
					printf("accept error!\n");
				} else {
					test->ev.events = EPOLLIN | EPOLLET;
					test->ev.data.fd = queue->client_sd;
					test->ev.data.ptr = (void *)queue;
					epoll_ctl(test->epoll_fd, EPOLL_CTL_ADD, queue->client_sd, &test->ev);
					queue_init(queue, (void *)test);
					list_add_tail(&test->sd_head, &queue->sd_list);
					//print_list(&test->sd_head);
					printf("add a client fd = %d\n", queue->client_sd);
				}
			} else if (test->polledevents[i].events & EPOLLIN) {
				int fd;
				if (test->polledevents[i].data.fd < 0)
					continue;

				if (test->polledevents[i].data.fd == 0)
					fd = 0;
				else
					fd = ((struct queue_head*)(test->polledevents[i].data.ptr))->client_sd;

				/* get count from fd */
				ioctl(fd, FIONREAD, &nread);

				if (test->polledevents[i].data.fd == 0) {
					/* get data from stdin */
					read(test->polledevents[i].data.fd, test->from_data, nread);
					if (strcmp(test->from_data, "q\n") == 0) {
						pthread_mutex_lock(&test->queue_mutex);
						struct list_head *temp = &test->sd_head;
						while (test->sd_head.next != &test->sd_head) {
							struct queue_head *list = container_of(temp->next, struct queue_head, sd_list);
							if (list->client_sd > 0) {
								/* tell client that server is going to quit */
								memset(test->to_data, 0, BUF_LEN);
								snprintf(test->to_data, BUF_LEN, "q");
								ret = write(list->client_sd, test->to_data, strlen(test->to_data));
								if (ret == -1) {
									printf("write to a client fd %d error!\n", list->client_sd);
								}
								printf("send to client %d: %s\n", list->client_sd, test->to_data);
								close(list->client_sd);
							}
							epoll_ctl(test->epoll_fd, EPOLL_CTL_DEL, list->client_sd, &test->ev);
							list_delete_self(&list->sd_list);
							list_delete_self(&list->queue_list);
							free(list);
						}
						flag = 1;
						kill_thread(queue_id, SIGUSR1);
						pthread_mutex_unlock(&test->queue_mutex);
						break;
					}
				} else {
					struct queue_head *queue = (struct queue_head*)test->polledevents[i].data.ptr;
					queue->nread = nread;
					/* any client fd disconnect */
					if (queue->nread == 0) {
						pthread_mutex_lock(&test->queue_mutex);
						close(queue->client_sd);
						list_delete_self(&queue->sd_list);
						list_delete_self(&queue->queue_list);
						printf("remove a client fd = %d\n", queue->client_sd);
						free(queue);
						pthread_mutex_unlock(&test->queue_mutex);
					} else {
						/* read data from client */
						memset(queue->from_data, 0, BUF_LEN);
						ret = read(queue->client_sd, queue->from_data, queue->nread);
						if (ret == -1) {
							printf("read from a client fd = %d error!\n", queue->client_sd);
						} else {
							printf("from a client fd = %d, received data: %s\n", queue->client_sd, queue->from_data);
						}

						/* enqueue */
						pthread_mutex_lock(&test->queue_mutex);
						enqueue(&test->queue_head, queue);
						sem_post(&test->queue_sem);
						pthread_mutex_unlock(&test->queue_mutex);
					}
				}
			} else {
				continue;
			}
		}
	}
	pthread_join(queue_id, NULL);
}

int tcp_server_exit(struct server_data *test)
{
	close(test->server_sd);
}

int main(int argc, char **argv)
{
	struct server_data test;
	int ret;

	ret = tcp_server_init(&test);
	if (ret == -1) {
		return ret;
	}

	tcp_handle_client_data(&test);

	tcp_server_exit(&test);
	return 0;
}
