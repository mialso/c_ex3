#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>		// close
#include <setjmp.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/time.h>		// FD_SET, FD_ZERO, FD_ISSET macros
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
//#include <arpa/inet.h>

#include <signal.h>

#include "server_error.h"
#include "server_socket.h"
#include "server_socket_list.h"
#include "fsm.h"

#define MESSAGE_SIZE 3

// static resources
static int port_num;
static int server_socket;
//static struct sockaddr_in address;
//static socklen_t addr_len;
static fd_set main_set;
//static fd_set primary_set;
//static int accepted_sock_num;
static struct fsm_server_sock_list accepted_socks;
static struct fsm_server_sock_list pre_primary_socks;
static int primary_sock;
static char buffer[MESSAGE_SIZE+1];

// pipe
static int pipe_fds[2];
// signals
static struct sigaction sig_act;

// error handler stuff
static jmp_buf goto_error;
static enum fsm_server_error_name server_error;

// service declaration
static void init_env(int argc, char *argv[]);
static void init_listener();
static void init_pipe();
static void signal_handler();
static void wait_all();
static void handle_signals();
static void accept_connections();
static void respond_accepted();
static void handle_primary();
static void non_fatal_error();
static void fatal_error();
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
		case PIPE_INIT_ERR:		fatal_error();
		case SIGNAL_INIT_ERR:		fatal_error();
		default:			fatal_error();
	}

	// init listener
	init_listener();
	// init pipe for signals to prevent select race
	init_pipe();

//main_loop:
	for (;;) 
	{
		wait_all();

		handle_signals();

		accept_connections();

		respond_accepted();
	
		handle_primary();
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
void init_listener() 
{
	/*int opt = 1;
	// create socket
	if (0 == (server_socket = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("socket failed");
		longjmp(goto_error, INIT_LISTENER_ERR);
	}
	// explicitly set to reuse address to avoid timeout
	if (0 > setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof opt)) {
		perror("setsockopt failed");
		longjmp(goto_error, INIT_LISTENER_ERR);
	}
	// fill in address
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port_num); 
	// bind socket
	if (0 > bind(server_socket, (struct sockaddr *) &address, sizeof address)) {
		perror("setsockopt failed");
		longjmp(goto_error, INIT_LISTENER_ERR);
	}
	addr_len = sizeof(address);
	// get port socket was bind to
	if (0 > getsockname(server_socket, (struct sockaddr * restrict) &address, &addr_len)) {
		perror("getsockopt failed");
		longjmp(goto_error, INIT_LISTENER_ERR);
	}
	// listen connections, up to 512 in queue (max number depends on OS and could be smaller)
	if (0 > listen(server_socket, 512)) {
		perror("listen");
		longjmp(goto_error, INIT_LISTENER_ERR);
	}*/
	if (-1 == (server_socket = get_passive_socket(&port_num))) {
		// unable to create listener
		printf("[LOG]: server failed to create listener...");
		exit(EXIT_FAILURE);
	}
	else {
		// info user about successful initialization
		printf("[INFO]: server listening at port %d\n", port_num);
	}
}
void init_pipe()
{
	int flags;
	if (-1 == pipe(pipe_fds)) {
		perror("pipe failed");
		longjmp(goto_error, PIPE_INIT_ERR);
	}
	// make read end non-blocking
	if (-1 == (flags = fcntl(pipe_fds[0], F_GETFL))) {
		perror("fcntl F_GETFL failed");
		longjmp(goto_error, PIPE_INIT_ERR);
	}
	flags |= O_NONBLOCK;
	if (-1 == fcntl(pipe_fds[0], F_SETFL, flags)) {
		perror("fcntl F_SETFL failed");
		longjmp(goto_error, PIPE_INIT_ERR);
	}
	// make write end non-blocking
	if (-1 == (flags = fcntl(pipe_fds[1], F_GETFL))) {
		perror("fcntl F_GETFL failed");
		longjmp(goto_error, PIPE_INIT_ERR);
	}
	flags |= O_NONBLOCK;
	if (-1 == fcntl(pipe_fds[1], F_SETFL, flags)) {
		perror("fcntl F_SETFL failed");
		longjmp(goto_error, PIPE_INIT_ERR);
	}
	// init signals
	sigemptyset(&sig_act.sa_mask);
	//sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = SA_RESTART;
	sig_act.sa_handler = signal_handler; 	// signal handler
	if (-1 == sigaction(SIGINT, &sig_act, NULL)) {
		perror("sigaction failed");
		longjmp(goto_error, SIGNAL_INIT_ERR);
	}

}
void signal_handler()
{
	int saved_errno;
	
	saved_errno = errno;
	if (-1 == write(pipe_fds[1], "x", 1) && errno != EAGAIN) {
		perror("signal_handler pipe write");
	}
	errno = saved_errno;
}
void wait_all() 
{
	int max_sd;
	int act;
	// clean & set up sets
	FD_ZERO(&main_set);
	FD_SET(server_socket, &main_set);
	max_sd = server_socket;
	// set pipe read-end to catch signals
	FD_SET(pipe_fds[0], &main_set);
	max_sd = (server_socket > pipe_fds[0]) ? server_socket : pipe_fds[0];
	// add accepted sockets if any
	add_accepted_socks();
		
	// add primary sockets if exist
	if (primary_sock) {
		FD_SET(primary_sock, &main_set);
	}
	act = select(max_sd + 1, &main_set, NULL, NULL, NULL);
	if ((0 > act) && (errno != EINTR)) {
		perror("select failure");
	}
	
}
void handle_signals()
{
	char ch = 0;
	if (FD_ISSET(pipe_fds[0], &main_set)) {
		printf("[LOG]: fsm_server: signal was caught\n");
		for (;;) {
			if (-1 == read(pipe_fds[0], &ch, 1)) {
				if (EAGAIN == errno) {
					return;		// nothing to do yet
					// break; 	// implement in case of real signals workflow
				}
				else {
					perror("handle_signals pipe read error");
				}
			}
		}
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
	int close_sock = 1;
	if ((sock = accepted_socks.first)) {
		do {
			if (FD_ISSET(sock->sd, &main_set)) {
				// check for full message availability
				// read all messages, put all socket who requested state to primary candidates
				if (0 < (mes_len = read(sock->sd, &buffer, MESSAGE_SIZE))) {
					if (mes_len == 2) {
						// check the message to be 'state request'
						if ('S' == buffer[0] && 'Q' == buffer[1]) {
							close_sock = sock_node_move(&accepted_socks, &pre_primary_socks, sock->sd);
						}
						else {
							// close all others
						}
					}
					else if (mes_len == 1) {
						// TODO partial message... need to handler it somehow
						// i can use recv instead of read with 'wait' flag
						// or ...
						printf("[LOG]: fsm_server partial message");
					}
					else {
						// it is not state request, ignore, ???close that socket
					}
				}
				else if (-1 == mes_len) {
					//error case
					perror("respond_accepted read error");
				}
				else {
					// disconnect case
				}
			}
			sock = sock->next;
			// close socket if not marked previously
			if (close_sock) {
				close(sock->prev->sd);
				sock_list_remove(&accepted_socks, sock->prev->sd);
			}
		} while (sock);
	}
}
void handle_primary() {
	enum fsm_state_name conv;	// for enum conversion???
	int msg_len;
	enum fsm_state_name resp;
	if (primary_sock) {
		if (FD_ISSET(primary_sock, &main_set)) {
			// read change state message
			if (0 > (msg_len = read(primary_sock, &buffer, MESSAGE_SIZE))) {
				perror("handle_primary primay_sock read error");
			}
			if (3 == msg_len) {
				if ('C' == buffer[0] && 'S' == buffer[1]) {
					// state request change
					fsm_switch_state(buffer[2]);
					fsm_current_state_name(&resp);
					buffer[0] = 'C';
					buffer[1] = 'R';
					buffer[2] = (char) resp;
					if (0 > (msg_len = write(primary_sock, &buffer, MESSAGE_SIZE))) {
						perror("handle_primary after state change write error");
					}
					else if (3 == msg_len) {
						// success write
						sock_list_push(&accepted_socks, primary_sock);
						primary_sock = 0;
					}
					else {
						// partially written? signal interrupted??
					}
				}
			}
			else if (3 > msg_len) {
				// partial message
				printf("[LOG]: fsm_server partial message");
			}
			else {
				// wrong message, close connection
			}
		}

	}
	else {
		// check for available pre_primary socks
		if (!(pre_primary_socks.first)) {
			return;
		}
		// make some sock primary
		primary_sock = pre_primary_socks.first->sd;
		sock_list_remove(&pre_primary_socks, primary_sock);
		
		buffer[0] = 'S';
		buffer[1] = 'R';
		//fsm_current_state_name((void *)(((char *)&buffer)+2));
		fsm_current_state_name(&conv);
		buffer[2] = (char) conv;
		if (0 > write(primary_sock, &buffer, MESSAGE_SIZE)) {
			if (EAGAIN == errno) {
				return; 	// may be re-try
			}
			else {
			 	perror("handle_primary write error");
			}
		}
	}
}
void non_fatal_error(enum fsm_server_error_name error_name)
{
	fprintf(stderr, "[LOG]: fsm_server: non_fatal_error = %d\n", error_name);
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
