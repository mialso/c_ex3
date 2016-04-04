#include <stdio.h>
// internal testing functionality
#include "tests.h"
// source to test
#include "../src/fsm.h"

// service declarations
struct test test_one;
struct test test_two;

// declarations
//extern struct fsm real_machine;
extern struct fsm_state real_state;
enum fsm_state_name test_fsm_state_name;

// tests
void test_fsm_current_state(enum fsm_state_name *state);

int main(int argc, char *argv[])
{
	
	test_one.name = "fsm_current_state";
	test_fsm_current_state(&test_fsm_state_name);
	tests_print_res(test_one);
	
	test_two.name = "fsm_switch_state";
	test_fsm_switch_state();
	tests_print_res(test_two);

	return 0;
}

void test_fsm_current_state(enum fsm_state_name *state)
{
	*state = EMPTY;
	if (0 == fsm_current_state_name(state)) {
		if (*state == real_state.name) {
			test_one.result = 1;
			return;
		}
		else {
			test_one.result = 0;
			test_one.error_message = "fail to get current state";
			return;
		}
	}
	else {
		test_one.result = 0;
		test_one.error_message = "error while getting current fsm state";
	}
}
