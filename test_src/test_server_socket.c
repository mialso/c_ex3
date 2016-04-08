#include <stdio.h>
#include <unistd.h>

#include "tests.h"

#include "../src/server_socket.h"

// tests data
static struct test test_one;

// tests procs
static void test_get_passive_socket();

// tests resources
static int socket;

int main(void)
{
	test_get_passive_socket();
	return 0;
}
void test_get_passive_socket()
{
	// test resources
	int port_num_in, port_num_out;
	// test with defined wright port number
	test_one.name = "get_passive_socket with wright port number";
	test_one.result = 1;
	port_num_in = 51234;
	port_num_out = port_num_in;
	if (-1 == (socket = get_passive_socket(&port_num_out))) {
		fill_failed_test(&test_one, -1);
 	}
	if (port_num_in != port_num_out) {
		test_one.error_message = "port numbers are not equal";
		fill_failed_test(&test_one, socket);
	}
	tests_print_res(test_one);

	// test with defined wrong port number
	test_one.name = "get_passive_socket with wrong port number";
	test_one.result = 1;
	port_num_in = 45544;
	port_num_out = port_num_in;
	if (-1 == (socket = get_passive_socket(&port_num_out))) {
		fill_failed_test(&test_one, -1);
 	}
	if (port_num_in == port_num_out) {
		test_one.error_message = "port numbers are equal";
		fill_failed_test(&test_one, port_num_out);
	}
	tests_print_res(test_one);
	
	// test with undefined port number
	test_one.name = "get_passive_socket without port number";
	test_one.result = 1;
	port_num_out = 0;
	if (-1 == (socket = get_passive_socket(&port_num_out))) {
		fill_failed_test(&test_one, -1);
 	}
	if (0 == port_num_out) {
		test_one.error_message = "port number is 0";
		fill_failed_test(&test_one, socket);
	}
	tests_print_res(test_one);
	if (socket) {
		close(socket);
	}
}
