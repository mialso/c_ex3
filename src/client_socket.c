#include <stdio.h>
#include <netdb.h>
#include <string.h> 	// memset
#include <errno.h>
#include <unistd.h>


int get_active_socket(char *host_name, char *port_num)
{
	int sd = 0;
	int res = 0;
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	
	// get addresses to connect
	memset (&hints, 0, sizeof (struct addrinfo));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;

	if (0 != (res = getaddrinfo(host_name, port_num, &hints, &result))) {
		//perror("get_active_socket getaddrinfo failed");
		fprintf(stderr, "[ERROR]: <client_socket>: get_active_socket(): getaddrinfo error: %s\n", gai_strerror(res));
		goto error;
	}
	// walk through res-list to find success connection
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (-1 == sd) {
			continue;	// error, try next address
		}
		if (-1 != connect(sd, rp->ai_addr, rp->ai_addrlen)) {
			break;		// success
		}
		// error, close this socket and try next addr
		close(sd);
	}
	if (0 == sd) {
		goto error; 	// no connection was found
	}
	freeaddrinfo(result);

	return sd;
	
error:
	if (sd) {
		close(sd);
	}
	return 0;
}
