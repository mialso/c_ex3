#ifndef _MLS_FSM_SERVER_ERROR_H
#define _MLS_FSM_SERVER_ERROR_H

enum fsm_server_error_name {
	OK,
	// initialization errors
	INIT_LISTENER_ERR,
	SELECT_ERR,
	ACCEPTION_FAIL
};

#endif
