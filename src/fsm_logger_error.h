#ifndef _MLS_FSM_LOGGER_ERROR_H
#define _MLS_FSM_LOGGER_ERROR_H

enum fsm_logger_error_name {
	OK,
	FILE_INIT,
	FILE_ERROR,
	MEMORY_ERROR,
	CLOCK_ERROR,
	CONVERT_ERROR,
	OUTPUT_ERROR
};

#endif
