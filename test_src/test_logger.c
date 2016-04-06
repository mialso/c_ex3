#include "tests.h"

// tests definition
struct test test_one;

// tests declarations
static void test_fsm_log(struct test t);

int main(void)
{
	test_fsm_log();

	return 0;
}

void test_fsm_log(struct test *test)
{
	test->name = "fsm_log";
	test->result = 0;
	
	tests_print_res(&test);
}
	
