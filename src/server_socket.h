#ifndef _MLS_SERVER_SOCKET_H
#define _MLS_SERVER_SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct passive_socket {
	int socket;
	struct sockaddr_in address;
	socklen_t addr_len;
};
	
int get_passive_socket(int *port_num);
int accept_conn_socket();
//int sock_read_message(int socket, char *buf, int mes_size);
int sock_read_state_req(int socket, int req_mes_len);
int is_partial(int socket, int mes_size);
int sock_send_response(int socket, char *buf, int size);
int sock_read_state_change(int socket, char *state);

#endif
