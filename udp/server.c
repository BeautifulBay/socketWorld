#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct server_data {
	int server_sd;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
#define SERVER_PORT 8888
#define SERVER_ADDR "127.0.0.1"
#define BUF_LEN 64
	char from_data[BUF_LEN];
	char to_data[BUF_LEN];
	int count;
	socklen_t len;
};

int udp_server_init(struct server_data *test)
{
	int ret;

	/* create socket fd */
	test->server_sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (test->server_sd < 0) {
		return test->server_sd;
	}

	/* config server addr */
	memset(&test->server_addr, 0 , sizeof(struct sockaddr_in));
	test->server_addr.sin_family = AF_INET;
	test->server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	test->server_addr.sin_port = htons(SERVER_PORT);

	/* init sockaddr_in len */
	test->len = sizeof(test->server_addr);

	/* bind socket fd to server addr */
	ret = bind(test->server_sd, (struct sockaddr *)&test->server_addr, test->len);
	if (ret < 0) {
		printf("server bind %s error!", inet_ntoa(test->server_addr.sin_addr));
		close(test->server_sd);
		return ret;
	}
}

int udp_handle_client_data(struct server_data *test)
{
	/* recvfrom data */
	/* 1 prepare buf */
	memset(test->from_data, 0, BUF_LEN);
	/* 2 get data to buf */
	test->count = recvfrom(test->server_sd, test->from_data, BUF_LEN, 0, (struct sockaddr*)&test->client_addr, &test->len);
	if (test->count == -1) {
		printf("recvfrom error!\n");
		return test->count;
	}

	/* 3 print data */
	printf("You received msg from %s : %s\n", inet_ntoa(test->client_addr.sin_addr), test->from_data);

	/* sendto data */
	/* 1 prepare buf */
	memset(test->to_data, 0, BUF_LEN);
	snprintf(test->to_data, BUF_LEN, "You mean ? %s", test->from_data);
	/* 2 send data */
	test->count = sendto(test->server_sd, test->to_data, BUF_LEN, 0, (struct sockaddr*)&test->client_addr, test->len);
	if (test->count == -1) {
		printf("send to %s error!\n", inet_ntoa(test->server_addr.sin_addr));
		return test->count;
	}

	/* break */
	if (strcmp(test->from_data, "q") == 0) {
		return 0;
	}
}

int udp_server_exit(struct server_data *test)
{
	/* close server sd */
	close(test->server_sd);
}

int main(int argc, char **argv)
{
	struct server_data test;
	int ret;

	udp_server_init(&test);

	while (1) {
		ret = udp_handle_client_data(&test);
		if (ret == 0) {
			break;
		}
	}

	udp_server_exit(&test);
	return 0;
}
