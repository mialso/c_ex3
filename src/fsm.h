#ifndef _MLS_FSM_H
#define _MLS_FSM_H

#include <stddef.h>

// declarations
enum fsm_state_name {
	EMPTY, 		// 0 by default
	DOWN = 68,	// ansi 'D'
	LEFT = 76,	// ansi 'L'
	RIGHT = 82,	// ansi 'R'
	UP = 85 	// ansi 'U'
};

struct fsm_state {
	enum fsm_state_name name;
	enum fsm_state_name possible[3];
	size_t next;
};

// definitions
struct fsm_state real_state;

// interface declaration
extern int fsm_current_state_name(enum fsm_state_name *name);
extern int fsm_switch_state(enum fsm_state_name name);
extern int fsm_flush_logs();

#endif
