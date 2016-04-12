#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "client_socket.h"

#define REQ_STATE "SQ"

// static res
static int sock;
static char host[128];
static char port[8];
static char mes_buf[4];

// service
static void init_env(int argc, char *argv[]);
static void connect_server(char *host, char *port);
static void request_state();
static void request_state_change();

int main(int argc, char *argv[])
{
	init_env(argc, argv);
	connect_server(host, port);
	request_state();
	request_state_change();
	if (sock) {
		close(sock);
	}
	
	exit(EXIT_SUCCESS);
}
void init_env(int argc, char *argv[])
{
	int port_num;
	//char buf[128];
	int scan_res;
	if (2 < argc) 
	{
		if (1 != (scan_res = sscanf(argv[1], "%s", host))) {
			printf("[ERROR]: host is not readable\n");
			goto usage;
		}
		if (1 != (scan_res = sscanf(argv[2], "%d", &port_num))) {
			printf("[ERROR]: port number is not readable\n");
			goto usage;
		}
		sprintf(port, "%d", port_num);
		return; // success
	}
		
usage:
	printf("[USAGE]: %s <host> <port>\n", argv[0]);
	exit(EXIT_FAILURE);
}
void connect_server(char *host, char *port)
{
	if (!(sock = get_active_socket(host, port))) {
		printf("[ERROR]: error establishing connection to %s:%s\n", host, port);
	}
}
void request_state()
{
	ssize_t res;
	if (-1 == (res = send(sock, REQ_STATE, 2, 0))) {
		perror("[ERROR]: request_state write error");
		exit(EXIT_FAILURE);
	}
	if (-1 == (res = recv(sock, mes_buf, 3, MSG_WAITALL))) {
		perror("[ERROR]: request_state read error");
		exit(EXIT_FAILURE);
	}
	if (0 == res) {
		printf("[ERROR]: req_state unexpected EOF from server\n");
	}
	printf("[INFO]: fsm state = %s\n", mes_buf);
}
void request_state_change()
{
	ssize_t res = 0;
	char buf[4] = {'C', 'S'};
	switch (mes_buf[2]) {
		case 'D':	buf[2] = 'L';
				break;
		case 'L':
		case 'R':	buf[2] = 'U';
				break;
		case 'U':	buf[2] = 'D';
				break;
		default:	break;
	}
	if (-1 == (res = send(sock, buf, 3, 0))) {
		perror("[ERROR]: request_state write error");
		exit(EXIT_FAILURE);
	}
	printf("[INFO]: %s success write, len = %ld\n", buf, res);
	if (-1 == (res = recv(sock, mes_buf, 3, MSG_WAITALL))) {
		perror("[ERROR]: request_state read error");
		exit(EXIT_FAILURE);
	}
	if (0 == res) {
		printf("[ERROR]: state_change unexpected EOF from server\n");
	}
	printf("[INFO]: fsm new state = %s\n", mes_buf);
}
		
