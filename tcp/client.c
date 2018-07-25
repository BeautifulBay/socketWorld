#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

int tcp_client_init(struct client_data *test)
{
	int ret;

	/* create client sd */
	test->client_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (test->client_sd < 0) {
		printf("socket client sd error!\n");
		return test->client_sd;
	}

	/* init */
	test->len = sizeof(test->server_addr);

	/* config server addr */
	memset(&test->server_addr, 0, sizeof(struct sockaddr_in));
	test->server_addr.sin_family = AF_INET;
	test->server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	test->server_addr.sin_port = htons(SERVER_PORT);

	/* client sd connect server addr*/
	ret = connect(test->client_sd, (struct sockaddr*)&test->server_addr, test->len);
	if(ret == -1) {
		printf("client connect error!\n");
		close(test->client_sd);
		return ret;
	}
}

int tcp_handle_server_data(struct client_data *test)
{
	int ret;

	/* prepare data to write */
	memset(test->to_data, 0, BUF_LEN);
	snprintf(test->to_data, BUF_LEN, "Hello!");
	printf("%s\n", test->to_data);

	/* write */
	ret = write(test->client_sd, test->to_data, strlen(test->to_data));
	if (ret == -1) {
		printf("sendto server %s error!\n", inet_ntoa(test->server_addr.sin_addr));
		return ret;
	}

	/* prepare buf to read */
	memset(test->from_data, 0, BUF_LEN);

	/* read */
	test->count = read(test->client_sd, test->from_data, BUF_LEN);
	if (test->count == -1) {
		printf("recvfrom server %s error!\n", inet_ntoa(test->server_addr.sin_addr));
		return test->count;
	}
	printf("You recieved %s: from %s\n", test->from_data, inet_ntoa(test->server_addr.sin_addr));
}

int tcp_client_exit(struct client_data *test)
{
	close(test->client_sd);
}

int main(int argc, char **argv)
{
	struct client_data test;
	int ret;

	ret = tcp_client_init(&test);
	if (ret < 0) {
		return ret;
	}

	tcp_handle_server_data(&test);

	tcp_client_exit(&test);

	return 0;
}
