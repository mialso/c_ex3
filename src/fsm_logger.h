#ifndef _MLS_FSM_LOGGER_H
#define _MLS_FSM_LOGGER_H

#include <stdint.h>

struct fsm_operation;
struct fsm_operation {
	struct fsm_operation *next;
	struct fsm_operation *prev;
	uint16_t time;
	uint16_t client_id;
	char type;
	char state;
	char error;
};

int fsm_log_file;
	
	
int fsm_log(int client_id, char type, char state, char error);
int fsm_logger_flush_logs();

#endif
