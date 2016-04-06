#ifndef _MLS_FSM_LOGGER_H
#define _MLS_FSM_LOGGER_H

#include <stdint.h>

struct fsm_operation {
	uint16_t time;
	uint16_t client_id;
	char type;
	char state;
	char error;
};
	
	
int fsm_log(int client_id, char type, char state, char error);

#endif
