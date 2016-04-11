#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h> 		// htons, ntohs
#include "tests.h"

#include "../src/client_socket.h"
#include "../src/server_socket.h"

#define S_CALL "netstat -pant 2>/dev/null | grep"
#define S_CALL_LEN 50
// tests
static struct test test_one;

// test res
static char *host_name = "localhost";
static char port_num[7];
static char buf[S_CALL_LEN];
static struct sockaddr_in address;
static socklen_t addr_len;

// tests procs
void test_get_active_socket();
int system_sock_test(char *port_num);

int main(void)
{
	test_get_active_socket();
		
	return 0;
}
void test_get_active_socket()
{
	int port;
	int server_socket;
	int client_socket;
	
	test_one.name = "get_active_socket wright port number";
	test_one.result = 1;
	port = 50001;
	if (-1 == (server_socket = get_passive_socket(&port))) {
		test_one.error_message = "server_socket error";
		fill_failed_test(&test_one, -1);
	}
	sprintf(port_num, "%d", port);
	if (-1 == (client_socket = get_active_socket(host_name, port_num))) {
		test_one.error_message = "client_socket error";
		fill_failed_test(&test_one, -1);
	}
	// obtaining client socket port
	if (0 > getsockname(client_socket, (struct sockaddr * restrict) &address, &addr_len)) {
		test_one.error_message = "getsockname failed";
		fill_failed_test(&test_one, -1);
	}
	printf("[INFO]: client socket port is: %d\n", ntohs(address.sin_port));
	//sprintf(port_num, "%d", ntohs(address.sin_port));
	if (-1 == system_sock_test(port_num)) {
		test_one.error_message = "netstat doesn't show port as active";
		fill_failed_test(&test_one, -1);
	}
	tests_print_res(test_one);

	if (server_socket) {
		close(server_socket);
	}
	if (client_socket) {
		close(client_socket);
	}
}
int system_sock_test(char *port_num)
{
	int call_len = sizeof S_CALL;
	strncpy(buf, S_CALL, call_len);
	buf[call_len-1] = ' ';
	strncpy(buf+call_len, port_num, strlen(port_num));
	//printf("system command: %s, len = %d\n", buf, call_len);
	return system(buf);
}
