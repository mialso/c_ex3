#ifndef _MLS_FSM_H
#define _MLS_FSM_H

// declarations
enum fsm_state_name {
	EMPTY, 		// 0 bt default
	DOWN = 68,	// ansi 'D'
	LEFT = 76,	// ansi 'L'
	RIGHT = 82,	// ansi 'R'
	UP = 85 	// ansi 'U'
};

struct fsm_state {
	enum fsm_state_name name;
};
/*struct fsm {
	struct fsm_state *state;
};*/

// definitions
struct fsm_state real_state;
/*struct fsm real_machine;*/

// interface declaration
int fsm_current_state_name(enum fsm_state_name *name);

// service declarations
void check_fsm() __attribute__ ((noreturn));
void log_error() __attribute__ ((noreturn));

#endif
