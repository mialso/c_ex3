#ifndef _MLS_FSM_SERVER_SOCKET_H
#define _MLS_FSM_SERVER_SOCKET_H

#include <stdlib.h>

struct fsm_server_sock_node;

struct fsm_server_sock_node {
	struct fsm_server_sock_node *next;
	struct fsm_server_sock_node *prev;
	int sd;
};

struct fsm_server_sock_list {
	struct fsm_server_sock_node *first;
	struct fsm_server_sock_node *last;
	//int count;
};

// interface
int sock_list_push(struct fsm_server_sock_list *list, int sd);
int sock_list_remove(struct fsm_server_sock_list *list, int sd);
int sock_node_move(struct fsm_server_sock_list * restrict l_from, 
			struct fsm_server_sock_list * restrict l_to, int sd);

#endif
