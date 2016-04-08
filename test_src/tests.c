#include <stdio.h>
#include "tests.h"

#define FAILED 0

// static declarations
static void print_error(struct test test_passed);
static void print_success(struct test test_passed);
static void print_internal_tests_error(struct test test_passed);

void tests_print_res(struct test test_passed)
{
	switch(test_passed.result) {
		case FAILED:	print_error(test_passed);
				break;
		case 1:		print_success(test_passed);
				break;
		default:	print_internal_tests_error(test_passed);
				break;
	}
}
void fill_failed_test(struct test *test, int err_num)
{
	test->result = FAILED;
	test->error_num = err_num;
}

void print_error(struct test test_passed)
{
	fprintf(stderr, "[TEST]: %s ERROR: %s, error_num = %d\n", test_passed.name, test_passed.error_message, test_passed.error_num); 
}
void print_success(struct test test_passed)
{
	printf("[TEST]: %s PASSED\n", test_passed.name); 
}
void print_internal_tests_error(struct test test_passed)
{
	fprintf(stderr, "[TEST]: INTERNAL ERROR while %s\n", test_passed.name);
}
