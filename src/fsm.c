#include <setjmp.h>
#include <stdio.h>
#include "fsm.h"

// error handling stuff
static int error;
// jumps stuff
static jmp_buf do_again;
static int guard;

int fsm_current_state_name(enum fsm_state_name *name)
{
	guard = setjmp(do_again);
	if (2 == guard) {
		return 1;
	}
	
	*name = real_state.name;
	if (*name) {
		return 0;
	}
	else {
		check_fsm();
	}
}
void check_fsm()
{
	static int only_one_init = 0;
	// if error, log and jump out to exit with error for client
	if (error) {
		log_error();
	}
	// else init new machine and jump to make next attempt to fetch state name
	real_state.name = DOWN;
	longjmp(do_again, ++only_one_init);
}
void log_error()
{
	// log
	fprintf(stderr, "[LOG]: fsm internal error: %d\n", error);
	// and jump to exit with error
	longjmp(do_again, 2);
}
