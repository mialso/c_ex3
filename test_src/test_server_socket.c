#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "tests.h"

#include "../src/server_socket.h"

#define S_CALL "netstat -plnt 2>/dev/null | grep"
#define S_CALL_LEN 50
// tests data
static struct test test_one;

// tests procs
static void test_get_passive_socket();
static int system_sock_test(char *port_num);

// tests resources
static int sockets[10];
static char buf[S_CALL_LEN];
static char port_num[7];

int main(void)
{
	test_get_passive_socket();
	return 0;
}
void test_get_passive_socket()
{
	// test resources
	int port_num_in, port_num_out;
	int i;
	// test with defined wright port number
	test_one.name = "get_passive_socket with wright port number";
	test_one.result = 1;
	port_num_in = 51234;
	port_num_out = port_num_in;
	if (-1 == (sockets[0] = get_passive_socket(&port_num_out))) {
		fill_failed_test(&test_one, -1);
 	}
	else if (port_num_in != port_num_out) {
		test_one.error_message = "port numbers are not equal";
		fill_failed_test(&test_one, sockets[0]);
	}
	sprintf(port_num, "%d", port_num_out);
	if (-1 == system_sock_test(port_num)) {
		test_one.error_message = "netstat doesn't show port as listening";
		fill_failed_test(&test_one, -1);
	}
	tests_print_res(test_one);

	// test with defined already in use port number
	test_one.name = "get_passive_socket with already in use port number";
	test_one.result = 1;
	port_num_in = 51234;
	port_num_out = port_num_in;
	if (-1 == (sockets[1] = get_passive_socket(&port_num_out))) {
		fill_failed_test(&test_one, -1);
 	}
	else if (port_num_in == port_num_out) {
		test_one.error_message = "port numbers are equal";
		fill_failed_test(&test_one, port_num_out);
	}
	sprintf(port_num, "%d", port_num_out);
	if (-1 == system_sock_test(port_num)) {
		test_one.error_message = "netstat doesn't show port as listening";
		fill_failed_test(&test_one, -1);
	}
	tests_print_res(test_one);
	
	// test with undefined port number
	test_one.name = "get_passive_socket without port number";
	test_one.result = 1;
	port_num_out = 0;
	if (-1 == (sockets[2] = get_passive_socket(&port_num_out))) {
		fill_failed_test(&test_one, -1);
 	}
	else if (0 == port_num_out) {
		test_one.error_message = "port number is 0";
		fill_failed_test(&test_one, sockets[3]);
	}
	sprintf(port_num, "%d", port_num_out);
	if (-1 == system_sock_test(port_num)) {
		test_one.error_message = "netstat doesn't show port as listening";
		fill_failed_test(&test_one, -1);
	}
	tests_print_res(test_one);
	for (i = 0; i < 10; ++i) {
		if (sockets[i]) {
			close(sockets[i]);
		}
	}
}
int system_sock_test(char *port_num)
{
	int call_len = sizeof S_CALL;
	strncpy(buf, S_CALL, call_len);
	buf[call_len-1] = ' ';
	strncpy(buf+call_len, port_num, strlen(port_num));
	printf("system command: %s, len = %d\n", buf, call_len);
	return system(buf);
}
