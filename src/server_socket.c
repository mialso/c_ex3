#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> 		// htons, ntohs

#include "server_socket.h"

int get_passive_socket(int *port_num)
{
	int opt = 1;
	int server_socket;
	struct sockaddr_in address;
	socklen_t addr_len;
	// create socket
	if (0 == (server_socket = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("get_passive_socket socket failed");
		return -1;
	}
	// explicitly set to reuse address to avoid timeout
	if (0 > setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof opt)) {
		perror("get_passive_socket setsockopt failed");
		return -1;
	}
	// fill in address
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(*port_num); 
	// bind socket
	if (0 > bind(server_socket, (struct sockaddr *) &address, sizeof address)) {
		perror("get_passive_socket setsockopt failed");
		return -1;
	}
	addr_len = sizeof(address);
	// get port socket was bind to
	if (0 > getsockname(server_socket, (struct sockaddr * restrict) &address, &addr_len)) {
		perror("get_passive_socket getsockopt failed");
		return -1;
	}
	// listen connections, up to 512 in queue (max number depends on OS and could be smaller)
	if (0 > listen(server_socket, 512)) {
		perror("get_passive_socket listen");
		return -1;
	}
	// info user about successful initialization
	*port_num = ntohs(address.sin_port);
	//printf("[INFO]: server listening at port %d\n", *port_num);
	return server_socket;
}
