#include <stdio.h>
#include <unistd.h>		// close
#include <stdlib.h>
#include <setjmp.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/time.h>		// FD_SET, FD_ZERO, FD_ISSET macros
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
//#include <arpa/inet.h>
#include <time.h>

#include <signal.h>

#include "server_error.h"
#include "server_socket.h"
#include "server_timer.h"
#include "server_socket_list.h"
#include "fsm.h"
//#include "fsm_error.h"

#define REQ_ST_MES_SIZE 2
#define REQ_CH_MES_SIZE 3

// wide available log file descriptor
extern int fsm_log_file;
// static resources
static int port_num;
static int server_socket;
//static struct sockaddr_in address;
//static socklen_t addr_len;
static fd_set main_set;
static fd_set write_set;
//static fd_set primary_set;
//static int accepted_sock_num;
static struct fsm_server_sock_list accepted_socks;
static struct fsm_server_sock_list pre_primary_socks;
static struct fsm_server_sock_list pre_cs_primary_socks;
static struct fsm_server_sock_list pre_cr_primary_socks;
//static int primary_sock;
static int sr_primary_sock;
static int cs_primary_sock;
static int cr_primary_sock;
//static char buffer[REQ_CH_MES_SIZE+1];
static int max_sd;

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
static void handle_fsm_logs();
static void serv_log(char *mes, int data);
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
	accepted_socks.ttl = 10;
	pre_primary_socks.ttl = 3;
	pre_cs_primary_socks.ttl = 3;
	pre_cr_primary_socks.ttl = 3;

	// init listener
	init_listener();
	// init pipe for signals to prevent select race
	init_pipe();
	// init timer
	if (OK != start_timer(500)) {
		fatal_error();
	}

//main_loop:
	for (;;) 
	{
		wait_all();

		handle_signals();

		accept_connections();

		respond_accepted();
	
		handle_primary();
		handle_fsm_logs();
	}
}
void init_env(int argc, char *argv[])
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
	if (-1 == (server_socket = get_passive_socket(&port_num))) {
		// unable to create listener
		fprintf(stderr, "[LOG]: server failed to create listener...");
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
	sig_act.sa_flags = SA_RESTART;
	sig_act.sa_handler = signal_handler; 	// signal handler
	if (-1 == sigaction(SIGUSR1, &sig_act, NULL)) {
		perror("sigaction failed");
		longjmp(goto_error, SIGNAL_INIT_ERR);
	}

}
void signal_handler()
{
	int saved_errno;
	int mes = 0;
	
	saved_errno = errno;
	
	if (-1 == (mes = write(pipe_fds[1], "x", 1))) {
		if (EAGAIN == errno) {
			serv_log("signal_handler EAGAIN", mes);
		}
		else {
			perror("signal_handler pipe write");
		}
	}
	serv_log("signal handler", mes);
	errno = saved_errno;
}
void wait_all() 
{
	serv_log("wait_all ....", 0);
	int act;
	max_sd = 0;
	// clean & set up sets
	FD_ZERO(&main_set);
	FD_ZERO(&write_set);
	FD_SET(server_socket, &main_set);
	max_sd = server_socket;
	// set pipe read-end to catch signals
	FD_SET(pipe_fds[0], &main_set);
	//max_sd = (max_sd > pipe_fds[0]) ? max_sd : pipe_fds[0];
	max_sd = (max_sd < pipe_fds[0]) ? pipe_fds[0] : max_sd;
	// add accepted sockets if any
	add_accepted_socks();
		
	// add primary sockets if exist
	if (sr_primary_sock) {
		FD_SET(sr_primary_sock, &write_set);
		max_sd = (max_sd > sr_primary_sock) ? max_sd : sr_primary_sock;
	}
	if (cs_primary_sock) {
		FD_SET(cs_primary_sock, &main_set);
		max_sd = (max_sd > cs_primary_sock) ? max_sd : cs_primary_sock;
	}
	if (cr_primary_sock) {
		FD_SET(cr_primary_sock, &write_set);
		max_sd = (max_sd > cr_primary_sock) ? max_sd : cr_primary_sock;
	}
	if (fsm_log_file) {
		FD_SET(fsm_log_file, &write_set);
		max_sd = (max_sd > fsm_log_file) ? max_sd : fsm_log_file;
	}
	act = select(max_sd + 1, &main_set, &write_set, NULL, NULL);
	if ((0 > act) && (errno != EINTR)) {
		perror("select failure");
	}
	serv_log("wait_all ....end", 0);
}
void handle_signals()
{
	serv_log("handle_signals ....", 0);
	char ch = 0;
	int updated = 0;
	if (FD_ISSET(pipe_fds[0], &main_set)) {
		serv_log("handle_signals: signals caught", 0);
		for (;;) {
		serv_log("handle_signals: in for loop ", 0);
		if (-1 == read(pipe_fds[0], &ch, 1)) {
			if (EAGAIN == errno) {
				break;
			}
			perror("handle_signals pipe read error");
			fatal_error();
		}
		if ('x' == ch && 0 == updated) {
			serv_log("handle_signals: x caught", 0);
			if (OK != sock_list_update_ttl(&accepted_socks)) {
				fatal_error();
			}
			if (OK != sock_list_update_ttl(&pre_primary_socks)) {
				fatal_error();
			}
			if (OK != sock_list_update_ttl(&pre_cs_primary_socks)) {
				fatal_error();
			}
			if (OK != sock_list_update_ttl(&pre_cr_primary_socks)) {
				fatal_error();
			}
			updated = 1;
		}
		}
	}
	serv_log("handle_signals ....end", 0);
}
void accept_connections() 
{
	serv_log("accept_connections ....", 0);
	int accepted_socket = 0;
	if (FD_ISSET(server_socket, &main_set)) 
	{
		if ((accepted_socket = accept_conn_socket())) {
			// add to accepted socket list
			serv_log("accept_connections new accepted_socket = ", accepted_socket);
			if (OK != sock_list_push(&accepted_socks, accepted_socket)) {
				fatal_error();
			}
		}
 	}
	serv_log("accept_connections ....end", 0);
}
void respond_accepted()
{
	serv_log("respond_accepted ....", 0);
	struct fsm_server_sock_node *sock = NULL;
	//int mes_len;
	int close_sock;
	if (!(sock = accepted_socks.first)) {
		return; 	// no accepted sockets
	}
	do {
		if (FD_ISSET(sock->sd, &main_set)) {
			serv_log("respond_accepted: socket data is ready", sock->sd);
			// check for partial messages
			if (!(is_partial(sock->sd, REQ_ST_MES_SIZE))) 
			{
				close_sock = sock_read_state_req(sock->sd, REQ_ST_MES_SIZE);
				if (close_sock) {
					serv_log("respond_accepted: wrong message ", sock->sd);
					// wrong message, close socket
					close(sock->sd);
					if (OK != sock_list_remove(&accepted_socks, sock->sd)) {
						fatal_error();
					}
				}
				else {
					// good state req, move to pre-primary
					if (OK != sock_node_move(&accepted_socks, &pre_primary_socks, sock->sd)) {
						fatal_error();
					}
				}
			}
		}
		sock = sock->next;
	} while (sock);
	serv_log("respond_accepted ....end", 0);
}
void handle_sr_primary()
{
	enum fsm_state_name conv;	// for enum conversion???
	char buffer[4] = {'S', 'R'};
	if (sr_primary_sock) 
	{
		if (FD_ISSET(sr_primary_sock, &write_set)) 
		{
			serv_log("handle_sr_primary(): data ready to be written", sr_primary_sock);
			// create state response ??? possible move to proc
			if (OK != fsm_current_state_name(&conv)) {
				fatal_error();
			}
			buffer[2] = (char) conv;
			if (OK != sock_send_response(sr_primary_sock, buffer, REQ_CH_MES_SIZE)) 
			{
				// error sending response
				serv_log("handle_sr_primary(): sock_send_response error", sr_primary_sock);
				close(sr_primary_sock);
				sr_primary_sock = 0;
				goto set_sr_primary;	// choose another one socket to communicate with
			}
			if (OK != sock_list_push(&pre_cs_primary_socks, sr_primary_sock)) {
				fatal_error();
			}
			sr_primary_sock = 0;
			goto set_sr_primary;
		}
	} 
	else 
	{
set_sr_primary:
		if (!(pre_primary_socks.first)) {
			return;
		}
		sr_primary_sock = pre_primary_socks.first->sd;
		if (OK != sock_list_remove(&pre_primary_socks, sr_primary_sock)) {
			fatal_error();
		}
	}
}
void handle_cs_primary()
{
	char req_state;
	//char buffer[4];
	if (cs_primary_sock)
	{
		if (FD_ISSET(cs_primary_sock, &main_set))
		{
			// read change state message
			serv_log("handle_cs_primary(): ready to read change state req", cs_primary_sock);
			if (is_partial(cs_primary_sock, REQ_CH_MES_SIZE)) {
				serv_log("handle_cs_primary(): partial  read", cs_primary_sock);
				return;
			}
			if (OK != sock_read_state_change(cs_primary_sock, &req_state)) {
				serv_log("handle_cs_primary(): error  read", cs_primary_sock);
				// error reading state
				close(cs_primary_sock);
				cs_primary_sock = 0;
				goto set_cs_primary;
			}
			// state change
			if (OK != fsm_switch_state(req_state)) {
				// wrong state was requested
				serv_log("handle_cs_primary(): wrong state", cs_primary_sock);
				close(cs_primary_sock);
				cs_primary_sock = 0;
				goto set_cs_primary;
			}
			if (OK != sock_list_push(&pre_cr_primary_socks, cs_primary_sock)) {
				fatal_error();
			}
			cs_primary_sock = 0;
			goto set_cs_primary;
		}
	}
	else 
	{
set_cs_primary:
		if (!(pre_cs_primary_socks.first)) {
			return;
		}
		cs_primary_sock = pre_cs_primary_socks.first->sd;
		if (OK != sock_list_remove(&pre_cs_primary_socks, cs_primary_sock)) {
			fatal_error();
		}
	}
}
void handle_cr_primary()
{
	enum fsm_state_name conv;	// for enum conversion???
	char buffer[4] = {'C', 'R'};
	if (cr_primary_sock) 
	{
		if (FD_ISSET(cr_primary_sock, &write_set)) 
		{
			serv_log("handle_cr_primary(): ready to write change state resp", cr_primary_sock);
			// create state response ??? possible move to proc
			if (OK != fsm_current_state_name(&conv)) {
				fatal_error();
			}
			buffer[2] = (char) conv;
			if (OK != sock_send_response(cr_primary_sock, buffer, REQ_CH_MES_SIZE)) 
			{
				// error sending response
				serv_log("handle_cr_primary(): sock_send_response error", cr_primary_sock);
				close(cr_primary_sock);
				cr_primary_sock = 0;
				goto set_cr_primary;	// choose another one socket to communicate with
			}
			if (OK != sock_list_push(&accepted_socks, cr_primary_sock)) {
				fatal_error();
			}
			cr_primary_sock = 0;
			goto set_cr_primary;
		}
	} 
	else 
	{
set_cr_primary:
		if (!(pre_cr_primary_socks.first)) {
			return;
		}
		cr_primary_sock = pre_cr_primary_socks.first->sd;
		if (OK != sock_list_remove(&pre_cr_primary_socks, cr_primary_sock)) {
			fatal_error();
		}
	}
}
void handle_primary()
{
	serv_log("handle_primary ....", 0);
	handle_sr_primary();
	handle_cs_primary();
	handle_cr_primary();
	serv_log("handle_primary ....end", 0);
}
void handle_fsm_logs()
{
	if (!fsm_log_file) {
		return;
	}
	if (FD_ISSET(fsm_log_file, &write_set)) {
		if (OK != fsm_flush_logs()) {
			fatal_error();
		}
	}
}
/*void handle_primary() {
	enum fsm_state_name conv;	// for enum conversion???
	enum fsm_state_name resp;
	if (primary_sock) {
	serv_log("handle_primary, primary sock is ", primary_sock);
		if (FD_ISSET(primary_sock, &main_set)) {
			fsm_current_state_name(&resp);
			buffer[0] = 'C';
			buffer[1] = 'R';
			buffer[2] = (char) resp;
			// write response with current fsm state
			if (sock_send_response(primary_sock, buffer, REQ_CH_MES_SIZE)) {
				// error sending response
				close(primary_sock);
				primary_sock = 0;
				goto set_primary;	// choose another one socket to communicate with
			}
		}

	}
	else {
		// set primary sock
set_primary:
		// check for available pre_primary socks
		if (!(pre_primary_socks.first)) {
			return;
		}
		serv_log("handle_primary(): before set_primary", pre_primary_socks.first->sd);
		// make some sock primary
		primary_sock = pre_primary_socks.first->sd;
		if (OK != sock_list_remove(&pre_primary_socks, primary_sock)) {
			fatal_error();
		}
		// create state response ??? possible move to proc
		buffer[0] = 'S';
		buffer[1] = 'R';
		if (OK != fsm_current_state_name(&conv)) {
			fatal_error();
		}
		buffer[2] = (char) conv;
		if (OK != sock_send_response(primary_sock, buffer, REQ_CH_MES_SIZE)) {
			// error sending response
			serv_log("handle_primary(): sock_send_response error", primary_sock);
			close(primary_sock);
			primary_sock = 0;
			goto set_primary;	// choose another one socket to communicate with
		}
	}
}*/
void serv_log(char *mes, int data)
{
	//fprintf(stderr, "[LOG]: fsm_server: %s, %d\n", mes, data);
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
			max_sd = (max_sd > sock->sd) ? max_sd : sock->sd;
			sock = sock->next;
		} while (sock);
	}
}
