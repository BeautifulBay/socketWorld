#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

struct client_data {
	int client_sd;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
#define SERVER_PORT 8888
#define SERVER_ADDR "127.0.0.1"
#define BUF_LEN     64
	char from_data[BUF_LEN];
	char to_data[BUF_LEN];
	int count;
	socklen_t len;
};

int udp_client_init(struct client_data *test)
{
	/* create client sd */
	test->client_sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (test->client_sd < 0) {
		printf("socket client sd error!\n");
		return test->client_sd;
	}

	/* init sockaddr_in len */
	test->len = sizeof(test->server_addr);

	/* config server addr */
	memset(&test->server_addr, 0, sizeof(struct sockaddr_in));
	test->server_addr.sin_family = AF_INET;
	test->server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	test->server_addr.sin_port = htons(SERVER_PORT);
}

int udp_handle_server_data(struct client_data *test, char data[])
{
	int ret;

	/* send data */
	/* 1 prepare buf */
	memset(test->to_data, 0, BUF_LEN);
	snprintf(test->to_data, BUF_LEN, "%s", data ? data : "Hello!");
	/* 2 print data */
	printf("%s\n", test->to_data);
	/* 3 send data */
	ret = sendto(test->client_sd, test->to_data, BUF_LEN, 0, (struct sockaddr*)&test->server_addr, test->len);
	if (ret < 0) {
		printf("sendto server %s error!\n", inet_ntoa(test->server_addr.sin_addr));
		return ret;
	}

	/* receive data */
	/* 1 get data */
	test->count = recvfrom(test->client_sd, test->from_data, BUF_LEN, 0, (struct sockaddr*)&test->server_addr, &test->len);
	if (test->count < 0) {
		printf("recvfrom server %s error!\n", inet_ntoa(test->server_addr.sin_addr));
		return test->count;
	}
	/* 2 print data */
	printf("You recieved msg from %s : %s\n", inet_ntoa(test->server_addr.sin_addr), test->from_data);
}

int udp_client_exit(struct client_data *test)
{
	/* close client sd */
	close(test->client_sd);
}

int main(int argc, char **argv)
{
	struct client_data test;
	int ret;

	ret = udp_client_init(&test);
	if (ret < 0) {
		return -1;
	}

	ret = udp_handle_server_data(&test, argv[1]);
	if(ret < 0) {
		printf("handle data error!\n");
	}

	udp_client_exit(&test);

	return 0;
}
