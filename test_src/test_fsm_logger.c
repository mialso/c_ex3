#include <unistd.h>
#include <stdlib.h>
#include "tests.h"
#include "../src/fsm_logger.h"


// tests definition
struct test test_one;
struct test test_two;

// tests declarations
static void test_fsm_log(struct test *t);
static void test_fsm_log_err(struct test *t);

int main(void)
{
	test_fsm_log(&test_one);
	test_fsm_log(&test_one);
	test_fsm_log_err(&test_two);
	test_fsm_log(&test_one);
	test_fsm_log(&test_one);
	sleep(3);
	test_fsm_log_err(&test_two);
	test_fsm_log_err(&test_two);
	sleep(3);
	test_fsm_log(&test_one);
	test_fsm_log(&test_one);

	exit(EXIT_SUCCESS);
}

void test_fsm_log(struct test *test)
{
	int res;
	test->name = "fsm_log";
	test->result = 0;
	
	if (0 == (res = fsm_log(111, 80, 65, 0))) {
		test->result = 1;
	}
	else {
		test->result = 0;
		test->error_message = "fsm_log failed to return success";
	}
	
	tests_print_res(*test);
}
void test_fsm_log_err(struct test *test)
{
	int res;
	test->name = "fsm_log";
	test->result = 0;
	
	if (0 == (res = fsm_log(222, 80, 65, 89))) {
		test->result = 1;
	}
	else {
		test->result = 0;
		test->error_message = "fsm_log failed to return success";
	}
	
	tests_print_res(*test);
}
	
