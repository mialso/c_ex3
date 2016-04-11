#include <stdio.h>
#include <unistd.h>
#include <errno.h>
//#include <sys/types.h>
//#include <sys/socket.h>
#include <arpa/inet.h> 		// htons, ntohs

#include "server_socket.h"

static struct passive_socket the_socket;

int get_passive_socket(int *port_num)
{
	int opt = 1;
	int addr_in_use = 0;
	//struct sockaddr_in address;
	//socklen_t addr_len;
	// create socket
	if (0 == (the_socket.socket = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("server_socket: get_passive_socket socket failed");
		goto error;
	}
	// explicitly set to reuse address to avoid timeout
	if (0 > setsockopt(the_socket.socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof opt)) {
		perror("server_socket: get_passive_socket setsockopt failed");
		goto error;
	}
bind:
	// fill in address
	the_socket.address.sin_family = AF_INET;
	the_socket.address.sin_addr.s_addr = INADDR_ANY;
	the_socket.address.sin_port = htons(*port_num); 
	// bind socket
	if (0 > bind(the_socket.socket, (struct sockaddr *) &the_socket.address, sizeof the_socket.address)) {
		if (EADDRINUSE == errno && 0 == addr_in_use) {
			// implemenet one more attempt
			*port_num = 0;
			addr_in_use = 1;
			goto bind;
		}
		perror("server_socket: get_passive_socket setsockopt failed");
		goto error;
	}
	the_socket.addr_len = sizeof the_socket.address;
	// get port socket was bind to
	if (0 > getsockname(the_socket.socket, (struct sockaddr * restrict) &the_socket.address, &the_socket.addr_len)) {
		perror("server_socket: get_passive_socket getsockname failed");
		goto error;
	}
	// listen connections, up to 512 in queue (max number depends on OS and could be smaller)
	if (0 > listen(the_socket.socket, 512)) {
		perror("server_socket: get_passive_socket listen failed");
		goto error;
	}
	// set socket port
	*port_num = ntohs(the_socket.address.sin_port);
	// TODO on exit clean up handler add

	return the_socket.socket;
error:
 	if (the_socket.socket) {
		close(the_socket.socket);
	}
	return -1;
}
int accept_conn_socket()
{
	int accepted_socket;
	if (0 > (accepted_socket = accept(the_socket.socket, (struct sockaddr *) &the_socket.address, &the_socket.addr_len))) {
		perror("server_socket: accept_conn_socket accept failed");
		return 0;
	}
	return accepted_socket;
}
int sock_read_state_req(int socket, int req_mes_len)
{
	char buf[4] = {'\0'};
	int mes_len;
	if (-1 == (mes_len = read(socket, buf, req_mes_len))) {
		perror("server_socket: sock_read_state_req() read");
		return 1;	// error
	}
	if (mes_len == req_mes_len) {
		if ('S' == buf[0] && 'Q' == buf[1]) {
			return 0; 	// success state request
		}
	}
	return 2;	// wrong message
}
int sock_read_state_change(int socket, char *state)
{
	char buf[5] = {'\0'};
	int req_mes_len = 3;
	ssize_t mes_len;
	if (-1 == (mes_len = recv(socket, buf, 3, MSG_WAITALL))) {
		perror("server_socket: sock_read_state_change() read");
		return 1;	// error
	}
	if (mes_len == req_mes_len) {
		printf("[INFO]: received %s len = %ld\n", buf, mes_len);
		if ('C' == buf[0] && 'S' == buf[1]) {
			switch (buf[2]) {
				case 'U':
				case 'D':
				case 'L':
				case 'R':	*state = buf[2];
						return 0; 	// success
				default:	return 2;	// wrong state requested
			}
		}
	}
	printf("[INFO]: error received %s len = %ld\n", buf, mes_len);
	return 3;	// wrong message
}
int sock_send_response(int socket, char *buf, int mes_len)
{
	int out_len;
	if (-1 == (out_len = send(socket, buf, mes_len, MSG_NOSIGNAL))) {
		perror("server_socket: sock_send_response() write");
		return 1;
	}
	if (mes_len == out_len) {
		printf("[INFO]: %s success written\n", buf);
		return 0; 	// success
	}
	return 2; 	// error, may be partial write possible, signal interrupted or smth else
}
int is_partial(int socket, int mes_size)
{
	char buf[5] = {'\0'};
	int ready_size;
	if (-1 == (ready_size = recv(socket, buf, mes_size, MSG_PEEK))) {
		perror("server_socket: check_partial recv error");
		return 1; 	// error
	}
	if (ready_size < mes_size) {
		printf("[LOG]: server_socket: partial message of size=%d '%s', sock = %d\n",ready_size, buf, socket);
		return 2;	// partial message
	}
	return 0;
}
