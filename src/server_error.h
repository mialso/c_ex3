#ifndef _MLS_FSM_SERVER_ERROR_H
#define _MLS_FSM_SERVER_ERROR_H

enum server_error_name {
	OK,
	// initialization errors
	INIT_LISTENER_ERR,
	PIPE_INIT_ERR,
	SIGNAL_INIT_ERR,
	ACCEPTION_FAIL,
	SELECT_ERROR,
	// module related
	FSM_FAIL,
	SOCKET_FAIL,
	SOCKET_LIST_FAIL,
	SIGNAL_FAIL
};

#endif
