#include "tests.h"
#include <stdio.h>
#include <stdlib.h>

#include "../src/server_socket_list.h"

// tests objects
static struct test test_one;
static struct test test_two;
static struct test test_three;

// static resources
static struct fsm_server_sock_list list_one;
static struct fsm_server_sock_list list_two;
static struct fsm_server_sock_list list_three;

// tests declarations
static void test_sock_list_push();
static void test_sock_list_move();
static void test_sock_list_pop();

int main(void)
{
	test_one.name = "sock_list_push";
	test_one.result = 1;
	test_sock_list_push();
	
	test_two.name = "sock_node_move";
	test_two.result = 1;
	test_sock_list_move();

	test_three.name = "sock_list_pop";
	test_three.result = 1;
	test_sock_list_pop();
	
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
void test_sock_list_move()
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
void test_sock_list_pop()
{
	int sd_1 = 2, sd_2 = 3, sd_3 = 15;
	int sd_poped;
	int res;
	res = sock_list_push(&list_three, sd_1);
	res = sock_list_push(&list_three, sd_2);
	res = sock_list_push(&list_three, sd_3);
	if (res) {
		test_three.result = 0;
		test_three.error_message = "sock_push error";
		test_three.error_num = res;
	}
	tests_print_res(test_three);
	
	res = sock_list_pop(&list_three, &sd_poped);
	if (15 != sd_poped) {
		test_three.result = 0;
		test_three.error_message = "sock_pop error";
		test_three.error_num = res;
	}
	tests_print_res(test_three);
}
