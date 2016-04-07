#include "tests.h"
#include <stdio.h>
#include <stdlib.h>

#include "../src/fsm_server_socket.h"

// tests objects
static struct test test_one;
static struct test test_two;

// static resources
static struct fsm_server_sock_list list_one;
static struct fsm_server_sock_list list_two;

// tests declarations
static void test_sock_list_push();
static void test_sock_list_move();

int main(void)
{
	test_one.name = "sock_list_push";
	test_one.result = 1;
	test_sock_list_push();
	
	test_two.name = "sock_node_move";
	test_two.result = 1;
	test_sock_list_move();
	
	exit(EXIT_SUCCESS);
}
void test_sock_list_push()
{
	int sd_1 = 123;
	int sd_2 = 34;
	int sd_3 = 0;
	int res;

	res = sock_list_push(&list_one, sd_1);
	if (res) {
		test_one.result = 0;
		test_one.error_message = "sock_list_push error";
		test_one.error_num = res;
	}
	tests_print_res(test_one);
	res = sock_list_push(&list_one, sd_1);
	if (7 != res) {
		test_one.result = 0;
		test_one.error_message = "sock_list_push the same value error";
		test_one.error_num = res;
	}
	tests_print_res(test_one);
	res = sock_list_push(&list_one, sd_2);
	if (res) {
		test_one.result = 0;
		test_one.error_message = "second sock_list_push error";
		test_one.error_num = res;
	}
	tests_print_res(test_one);
	res = sock_list_push(&list_one, sd_3);
	if (2 != res) {
		test_one.result = 0;
		test_one.error_message = "sock_list_push sd = 0 error";
		test_one.error_num = res;
	}
	tests_print_res(test_one);
	res = sock_list_remove(&list_one, sd_1);
	if (res) {
		test_one.result = 0;
		test_one.error_message = "sock_list_remove error";
		test_one.error_num = res;
	}
	tests_print_res(test_one);
	res = sock_list_remove(&list_one, sd_3);
	if (3 != res) {
		test_one.result = 0;
		test_one.error_message = "sock_list_remove absent node error";
		test_one.error_num = res;
	}
	tests_print_res(test_one);
	res = sock_list_remove(&list_one, sd_2);
	if (res) {
		test_one.result = 0;
		test_one.error_message = "sock_list_remove last node error";
		test_one.error_num = res;
	}
	tests_print_res(test_one);
	if ((NULL == list_one.first) && (NULL == list_one.last)) {
		test_one.result = 1;
		test_one.error_message = NULL;
	}
	else {
		test_one.result = 0;
		test_one.error_message = "test_one list not empty";
	}
	tests_print_res(test_one);
}
static void test_sock_list_move()
{
	int i;
	int res;

	for (i = 1; i < 11; ++i) {
		res = sock_list_push(&list_one, i);
		if (res) {
			test_two.result = 0;
			test_two.error_message = "sock_list_push error";
			test_two.error_num = res;
		}
	}
	tests_print_res(test_two);
	res = sock_node_move(&list_one, &list_two, 2);
	if (res) {
		test_two.result = 0;
		test_two.error_message = "sock_node_move error";
		test_two.error_num = res;
	}
	tests_print_res(test_two);
	res = sock_node_move(&list_one, &list_two, 5);
	if (res) {
		test_two.result = 0;
		test_two.error_message = "sock_node_move error";
		test_two.error_num = res;
	}
	tests_print_res(test_two);
	res = sock_node_move(&list_one, &list_two, 10);
	if (res) {
		test_two.result = 0;
		test_two.error_message = "sock_node_move error";
		test_two.error_num = res;
	}
	tests_print_res(test_two);
	res = sock_list_remove(&list_two, 5);
	if (res) {
		test_two.result = 0;
		test_two.error_message = "sock_node_remove error";
		test_two.error_num = res;
	}
	tests_print_res(test_two);
	res = sock_list_remove(&list_one, 5);
	if (3 != res) {
		test_two.result = 0;
		test_two.error_message = "sock_node_remove error";
		test_two.error_num = res;
	}
	tests_print_res(test_two);
}
