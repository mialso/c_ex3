#include <setjmp.h>
#include <stdio.h>
#include "fsm.h"
#include "fsm_error.h"
#include "fsm_logger.h"

#define STATE_REQ 'S'
#define STATE_CHANGE 'C'
#define STATE_ERROR 'E'
#define STATE_NO_ERROR ' '

// internal declarations
static void switch_state(enum fsm_state_name name);
//static void update_possible();
static void set_possible(enum fsm_state_name one, enum fsm_state_name two, enum fsm_state_name three);
static void check_fsm() __attribute__ ((noreturn));
//static void log_error(); 	// TODO implement logging

// error handling stuff
static enum fsm_error_name fsm_error;
//int error;

// jumps stuff
jmp_buf do_again;

int fsm_current_state_name(enum fsm_state_name *name)
{
	// error handler initialization
	fsm_error = setjmp(do_again);
	if (INIT < fsm_error) {
		return fsm_error;
	}
	
	*name = real_state.name;
	if (*name) {
		if (OK != fsm_log(1000, STATE_REQ, real_state.name, STATE_NO_ERROR)) {
			fsm_error = FSM_LOGGER_ERROR;
			return fsm_error;
		}
		return OK;
	}
	else {
		// try to init new machine in case of first request
		check_fsm();
	}
}
int fsm_switch_state(enum fsm_state_name name)
{
	int i;
	// error handler initialization
	fsm_error = setjmp(do_again);
	if (INIT < fsm_error) {
		return fsm_error;
	}
	// searching for requested state_name in possible values for current state and switch
	for (i = 0; i < 3; ++i) {
		if (name == real_state.possible[i]) {
			switch_state(name);
			if (OK != fsm_log(1000 , STATE_CHANGE, real_state.name, STATE_NO_ERROR)) {
				fsm_error = FSM_LOGGER_ERROR;
				return fsm_error;
			}
			return OK;
		}
	}
	// no match among possible state names was found, error
	fsm_error = IMPOSSIBLE_STATE;
	return IMPOSSIBLE_STATE;
}
int fsm_flush_logs()
{
	int res;
	res = fsm_logger_flush_logs();
	return res;
}
void switch_state(enum fsm_state_name name)
{
	switch(name) {
		case DOWN:	set_possible(UP, LEFT, RIGHT);
				break;
		case RIGHT:	
		case LEFT:	set_possible(UP, EMPTY, EMPTY);
				break;
		case UP: 	set_possible(DOWN, EMPTY, EMPTY);
				break;
		// error case, jump out
		default:	longjmp(do_again, WRONG_STATE_NAME);
	}
	real_state.name = name;
}
void set_possible(enum fsm_state_name one, enum fsm_state_name two, enum fsm_state_name three)
{
	if (one) {
		real_state.possible[0] = one;
		if (two) {
			real_state.possible[1] = two;
		}
		else {
			real_state.possible[1] = one;
		}
		if (three) {
			real_state.possible[2] = three;
		}
		else {
			real_state.possible[2] = one;
		}
	}
	else {
		longjmp(do_again, FSM_MATRIX_ERROR);
	}
}
void check_fsm()
{
	// if error, log and jump out to exit with error for client
	if (fsm_error) {
		longjmp(do_again, fsm_error);
	}
	// else init new machine and jump to make next attempt to fetch state name
	switch_state(DOWN);
	longjmp(do_again, INIT);
}
