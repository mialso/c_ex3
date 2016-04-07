#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>		// close
#include <setjmp.h>

#include <sys/time.h>		// FD_SET, FD_ZERO, FD_ISSET macros
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "fsm_server_error.h"
#include "fsm_server_socket.h"

#define MESSAGE_SIZE 3

// static resources
static int port_num;
static int server_socket;
static struct sockaddr_in address;
static socklen_t addr_len;
static fd_set main_set;
static fd_set primary_set;
//static int accepted_sock_num;
static struct fsm_server_sock_list accepted_socks;
static int primary_sock;
static char buffer[MESSAGE_SIZE+1];

// error handler stuff
static jmp_buf goto_error;
static enum fsm_server_error_name server_error;

// service declaration
static void init_env(int argc, char *argv[]);
static void init_listener();
static void accept_connections();
static void wait_all();
static void respond_accepted();
static void wait_primary();
static void fatal_error() __attribute__((noreturn));
static void add_accepted_socks();

int main(int argc, char *argv[])
{
	// init arguments -> create environment
	init_env(argc, argv);

	// handle errors
	server_error = setjmp(goto_error);
	switch (server_error) {
		case OK:			break;
		case INIT_LISTENER_ERR:		fatal_error();
		case SELECT_FAILURE: 		fatal_error();
		default:			fatal_error();
	}

	// init listener
	init_listener();

//main_loop:
	for (;;) 
	{
		wait_all();

		accept_connections();

		respond_accepted();
	
		wait_primary();

		exit(EXIT_SUCCESS);
	}
}
static void init_env(int argc, char *argv[])
{
	int scan_res;
	if (1 < argc) {
		if (1 == (scan_res = sscanf(argv[1], "%d", &port_num))) {
			if (50000 <= port_num && port_num <= 55000) {
				return;
			}
			else {
				fprintf(stderr, "[LOG]: port number should be in range [50,000..55,000]\n");
			}
		}
		else {
			fprintf(stderr, "[LOG]: unable to scan port number\n");
		}
	}
	fprintf(stderr, "[USAGE]: %s <port>\n", argv[0]);
	fprintf(stderr, "[LOG]: Starting without specified port number...\n");
	port_num = 0;
}
static void init_listener() 
{
	int opt = 1;
	// create socket
	if (0 == (server_socket = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("socket failed");
		longjmp(goto_error, INIT_LISTENER_ERR);
	}
	if (0 > (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof opt))) {
		perror("setsockopt failed");
		longjmp(goto_error, INIT_LISTENER_ERR);
	}
	// fill in address
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port_num); 
	// bind socket
	if (0 > (bind(server_socket, (struct sockaddr *) &address, sizeof address))) {
		perror("setsockopt failed");
		longjmp(goto_error, INIT_LISTENER_ERR);
	}
	addr_len = sizeof(address);
	// get port socket was bind to
	if (0 > (getsockname(server_socket, (struct sockaddr * restrict) &address, &addr_len))){
		perror("getsockopt failed");
		longjmp(goto_error, INIT_LISTENER_ERR);
	}
	// info user about successful initialization
	printf("[INFO]: server listening at port %d\n", ntohs(address.sin_port));
}
static void wait_all() 
{
	int max_sd;
	int act;
	// clean & set up sets
	FD_ZERO(&main_set);
	FD_SET(server_socket, &main_set);
	max_sd = server_socket;
	// add accepted sockets if any
	add_accepted_socks();
		
	// add primary sockets if exist
	if (primary_sock) {
		FD_SET(primary_sock, &main_set);
	}
	act = select(max_sd + 1, &main_set, NULL, NULL, NULL);
	if ((0 > act) && (errno != EINTR)) {
		printf("select failure");
		longjmp(goto_error, SELECT_ERR);
	}
	
}
void accept_connections() 
{
	int accepted_socket = 0;
	if (FD_ISSET(server_socket, &main_set)) 
	{
		if (0 > (accepted_socket = accept(server_socket, (struct sockaddr *) &address, &addr_len))) {
			perror("accept failed");
			non_fatal_error(ACCEPTION_FAIL);
		}
		else {
			// add to accepted socket list
			sock_list_push(&accepted_socks, accepted_socket);
		}
 	}
}
void respond_accepted()
{
	struct fsm_server_sock_node *sock = NULL;
	int mes_len;
	if ((sock = accepted_list.first)) {
		do {
			if (FD_ISSET(sock->sd, &main_set)) {
				// read all messages, put all socket who requested state to primary candidates
				if (0 < (mes_len = read(sock->sd, buffer, MESSAGE_SIZE))) {
					if (mes_len == 2) {
						// check the message to be 'state request'
					}
					else if (mes_len == 1) {
						// partial message... need to handler it somehow
					}
					else {
						// it is not state request, ignore, ???close that socket
						close(sock->sd);
						sock_list_remove(&accepted_socks, sock->sd);
					}
					
				}
				else if (-1 == mes_len) {
					//error case
				}
				else {
					// disconnect case
					close(sock->sd);
					sock_list_remove(&accepted_socks, sock->sd);
				}
			}
			sock = sock->next;
		} while (sock);
	}
}
void wait_primary() {
	if ((primary_sock) && FD_ISSET(primary_sock, &main_set)) {
		// read change state message
		

	}
}
void non_fatal_error(enum fsm_server_error_name error_name)
{
	fprintf("[LOG]: fsm_server: non_fatal_error = %d\n", error_name);
}
void fatal_error()
{
	fprintf(stderr, "[LOG]: fsm_server: fatal_error = %d\n", server_error);
	exit(EXIT_FAILURE);
}
void add_accepted_socks()
{
	struct fsm_server_sock_node *sock = NULL;
	if ((sock = accepted_socks.first)) {
		do {
			FD_SET(sock->sd, &main_set);
			sock = sock->next;
		} while (sock);
	}
}
