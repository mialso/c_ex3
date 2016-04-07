#ifndef _MLS_FSM_SERVER_H
#define _MLS_FSM_SERVER_H

struct fsm_server_socket {
	struct fsm_server_socket *prev;
	struct fsm_server_socket *next;
	int sock_fd;
};
#endif
