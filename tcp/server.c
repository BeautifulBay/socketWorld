#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct server_data {
#define SERVER_PORT 8888
#define SERVER_ADDR "127.0.0.1"
#define BUF_LEN 	64
#define CLIENT_NUM 	64
	int server_sd;
	int client_sd[CLIENT_NUM];
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr[CLIENT_NUM];
	char from_data[BUF_LEN];
	char to_data[BUF_LEN];
	int count;
	int index;
	int len;
};

int tcp_server_init(struct server_data *test)
{
	int ret;
	/* create socket fd */
	test->server_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (test->server_sd == -1) {
		return test->server_sd;
	}

	/* init */
	test->index = 0;
	test->len = sizeof(struct sockaddr_in);
	memset(test->client_sd, 0, CLIENT_NUM);

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
}

int tcp_handle_client_data(struct server_data *test)
{
	int fd_max = test->server_sd;
	int ret;
	int fd;
	int nread;
	int i;

	fd_set rset, bset;
	FD_ZERO(&bset);
	FD_ZERO(&rset);
	FD_SET(test->server_sd, &bset);
	FD_SET(0, &bset);

	while(1) {
		rset = bset;
		ret = select(fd_max + 1, &rset, NULL, NULL, 0);
		if (ret == -1) {
			printf("select error!\n");
			break;
		}
		for (fd = 0; fd <= fd_max; fd++) {
			if (FD_ISSET(fd, &rset)) {
				/* client sd connect */
				if (fd == test->server_sd) {
					if (test->index < CLIENT_NUM) {
						test->client_sd[test->index] = accept(test->server_sd, (struct sockaddr*)&test->client_addr[test->index], &test->len);
						if (test->client_sd[test->index] == -1) {
							printf("accept error!\n");
						} else {
							FD_SET(test->client_sd[test->index], &bset);
							printf("add a client fd = %d, index = %d\n", test->client_sd[test->index], test->index);
							fd_max = test->client_sd[test->index];
							test->index++;
						}
					} else {
						printf("There is no room!\n");
					}
				} else {
					/* get count from fd */
					ioctl(fd, FIONREAD, &nread);

					/* any client fd disconnect */
					if (nread == 0) {
						close(fd);
						FD_CLR(fd, &bset);
						printf("remove a client fd = %d\n", fd);
						for (i = 0; i < CLIENT_NUM; i++) {
							if (test->client_sd[i] == fd) {
								test->client_sd[i] = 0;
							}
						}
					} else {
						/* read data from fd */
						memset(test->from_data, 0, BUF_LEN);
						ret = read(fd, test->from_data, nread);
						if (ret == -1) {
							printf("read from a client fd = %d error!\n", fd);
						} else {
							printf("from a client fd = %d, received data: %s\n", fd, test->from_data);
							/* get data from stdin */
							if (fd == 0) {
								if (strcmp(test->from_data, "q\n") == 0) {
									for (i = 0; i < CLIENT_NUM; i++) {
										if (test->client_sd[i] != 0) {
											test->client_sd[i] = 0;
											close(test->client_sd[i]);
										}
									}
									return 0;
								}
							} else {
								/* get data from other client sd */
								memset(test->to_data, 0, BUF_LEN);
								snprintf(test->to_data, BUF_LEN, "Good job!");
								ret = write(fd, test->to_data, strlen(test->to_data));
								if (ret == -1) {
									printf("write to a client fd %d error!\n", fd);
								}
							}
						}
					}
				}
			}
		}
	}
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
