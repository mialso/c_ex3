#ifndef _MLS_TESTS_H
#define _MLS_TESTS_H

struct test {
	char *name;
	char *error_message;
	int result;
	int error_num;
};

void tests_print_res(struct test test_passed);
void fill_failed_test(struct test *test, int err_num);

#endif
