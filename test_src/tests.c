#include <stdio.h>
#include "tests.h"

// static declarations
void print_error(struct test test_passed);
void print_success(struct test test_passed);
void print_internal_tests_error(struct test test_passed);

void tests_print_res(struct test test_passed)
{
	switch(test_passed.result) {
		case 0:		print_error(test_passed);
				break;
		case 1:		print_success(test_passed);
				break;
		default:	print_internal_tests_error(test_passed);
				break;
	}
}

void print_error(struct test test_passed)
{
	fprintf(stderr, "[test]: %s ERROR: %s\n", test_passed.name, test_passed.error_message); 
}
void print_success(struct test test_passed)
{
	printf("[test]: %s PASSED\n", test_passed.name); 
}
void print_internal_tests_error(struct test test_passed)
{
	fprintf(stderr, "[test]: INTERNAL ERROR while %s\n", test_passed.name);
}
