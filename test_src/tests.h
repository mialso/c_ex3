#ifndef _MLS_TESTS_H
#define _MLS_TESTS_H

struct test {
	char *name;
	int result;
	char *error_message;
};

void tests_print_res(struct test test_passed);
void perform_test(struct test test_me, char *name, void *proc);

#endif