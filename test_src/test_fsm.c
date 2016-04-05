#include <stdio.h>
// internal testing functionality
#include "tests.h"
// source to test
#include "../src/fsm.h"

// declarations
extern struct fsm_state real_state;

// tests declarations
static struct test test_one;
static struct test test_two;
static struct test test_three;

// tests logic
static void test_fsm_current_state(enum fsm_state_name *state);
static void test_success_fsm_switch_state(enum fsm_state_name name);
static void test_error_fsm_switch_state(enum fsm_state_name new_state);

int main(void)
{
	enum fsm_state_name test_one_fsm_state_name;
	
	test_one.name = "fsm_current_state";
	test_fsm_current_state(&test_one_fsm_state_name);
	tests_print_res(test_one);
	
	test_two.name = "fsm_switch_state (success)";
	test_success_fsm_switch_state(LEFT);
	tests_print_res(test_two);

	test_one.name = "fsm_current_state";
	test_fsm_current_state(&test_one_fsm_state_name);
	tests_print_res(test_one);

	test_three.name = "fsm_switch_state (error)";
	test_error_fsm_switch_state(0);
	tests_print_res(test_three);

	test_one.name = "fsm_current_state";
	test_fsm_current_state(&test_one_fsm_state_name);
	tests_print_res(test_one);
	return 0;
}

void test_fsm_current_state(enum fsm_state_name *state)
{
	*state = EMPTY;
	if (0 == fsm_current_state_name(state)) {
		if (*state == real_state.name  && EMPTY != real_state.name) {
			test_one.result = 1;
			return;
		}
		else if (EMPTY == *state) {
			test_one.result = 0;
			test_one.error_message = "EMPTY state return detected";
			return;
		}
		else {
			test_one.result = 0;
			test_one.error_message = "real and returned states compare fails";
			return;
		}
	}
	else {
		test_one.result = 0;
		test_one.error_message = "error while getting current fsm state";
	}
}
void test_success_fsm_switch_state(enum fsm_state_name new_state)
{
	if (0 == fsm_switch_state(new_state)) {
		if (real_state.name == new_state) {
			test_two.result = 1;
			return;
		}
		else {
			test_two.result = 0;
			test_two.error_message = "real and new state compare fails";
			return;
		}
	}
	else {
		test_two.result = 0;
		test_two.error_message = "error while switching state";
		return;
	}
}
void test_error_fsm_switch_state(enum fsm_state_name new_state)
{
	int res;
	if ((res = fsm_switch_state(new_state))) {
		test_three.result = 1;
		return;
	}
	else {
		test_three.result = 0;
		test_three.error_message = "switch state error-test returned success";
		return;
	}
}
