#include <stdio.h>
#include <unistd.h>		// close
#include <stdlib.h>
#include <errno.h>

#include <sys/time.h>		// FD_SET, FD_ZERO, FD_ISSET macros

#include "server_error.h"
#include "server_socket.h"
#include "server_signal.h"
#include "server_socket_list.h"
#include "fsm.h"

// define connections time-to-live
#define WAIT_TTL 10;
#define ACT_TTL 3;

// global descriptors to handle by select
extern int fsm_log_file;
extern int pipe_read_fd;
// static resources
static int port_num;
static int server_socket;

static fd_set main_set;
static fd_set write_set;

static struct fsm_server_sock_list accepted_socks;
static struct fsm_server_sock_list sr_socks;
static struct fsm_server_sock_list cs_socks;
static struct fsm_server_sock_list cr_socks;
static int max_sd;

// service declaration
static void init_env(int argc, char *argv[]);
static void init_listener();
static void wait_all();
static void handle_signals();
static void accept_connections();
static void respond_accepted();
static void handle_primary();
static void handle_sr_primary();
static void handle_cs_primary();
static void handle_cr_primary();
static void handle_fsm_logs();
static void serv_log(char *mes, int data);
static void fatal_error(enum server_error_name err_name);
static void add_accepted_socks();
static void add_fd_to_set(int fd, fd_set *set);
static void remove_connection(int sock, struct fsm_server_sock_list *list);
static void log_server_state();

int main(int argc, char *argv[])
{
	// init arguments -> create environment
	init_env(argc, argv);

	// init passive socket - listener
	init_listener();
	// init signal pipe 
	init_signal_pipe();
	// init connection ttl (socket_lists)
	accepted_socks.ttl = WAIT_TTL;
	sr_socks.ttl = ACT_TTL;
	cs_socks.ttl = ACT_TTL;
	cr_socks.ttl = ACT_TTL;

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
			fprintf(stderr, "[LOG]: port number should be in range [50,000..55,000]\n");
		}
		fprintf(stderr, "[LOG]: unable to scan port number\n");
	}
	fprintf(stderr, "[USAGE]: %s <port>\n", argv[0]);
	fprintf(stderr, "[LOG]: Starting without specified port number...\n");
	port_num = 0;
	
}
void init_listener() 
{
	if (-1 == (server_socket = get_passive_socket(&port_num))) {
		fatal_error(SOCKET_FAIL);
	}
	// info user about successful initialization
	printf("[INFO]: server listening at port %d\n", port_num);
}
void wait_all() 
{
	int act;
	max_sd = STDERR_FILENO;	// to prevent select freeze std-channels communication
	// clean & set up sets
	FD_ZERO(&main_set);
	FD_ZERO(&write_set);
	// service internal fd's
	add_fd_to_set(server_socket, &main_set);
	add_fd_to_set(pipe_read_fd, &main_set);
	add_fd_to_set(fsm_log_file, &write_set);
	// client fd's
	add_accepted_socks();
	if (sr_socks.first) {
		add_fd_to_set(sr_socks.first->sd, &write_set);
	}
	if (cs_socks.first) {
		add_fd_to_set(cs_socks.first->sd, &main_set);
	}
	if (cr_socks.first) {
		add_fd_to_set(cr_socks.first->sd, &write_set);
	}
	// init select
	act = select(max_sd + 1, &main_set, &write_set, NULL, NULL);
	if ((0 > act) && (errno != EINTR)) {
		perror("select failure");
		fatal_error(SELECT_ERROR);
	}
}
void handle_signals()
{
	char ch = 0;
	int updated = 0;
	static int counter;
	if (!(FD_ISSET(pipe_read_fd, &main_set))) {
		return;
	}
	for (;;) {
		// TODO possibly refactor to get number of events and update ttl according to that
		if (-1 == read(pipe_read_fd, &ch, 1)) {
			if (EAGAIN == errno) {
				break;
			}
			perror("handle_signals pipe read error");
			fatal_error(SIGNAL_FAIL);
		}
		if (timer_signal == ch && 0 == updated) {
			if (OK != sock_list_update_ttl(&accepted_socks)) {
				fatal_error(SOCKET_LIST_FAIL);
			}
			if (OK != sock_list_update_ttl(&sr_socks)) {
				fatal_error(SOCKET_LIST_FAIL);
			}
			if (OK != sock_list_update_ttl(&cs_socks)) {
				fatal_error(SOCKET_LIST_FAIL);
			}
			if (OK != sock_list_update_ttl(&cr_socks)) {
				fatal_error(SOCKET_LIST_FAIL);
			}
			updated = 1;
			++counter;
		}
	}
	if (3 == (counter & 3)) {	// simple 4%
		log_server_state();
	}
}
void accept_connections() 
{
	int accepted_socket = 0;
	if (!FD_ISSET(server_socket, &main_set)) {
		return;
	}
	if ((accepted_socket = accept_conn_socket())) {
		// add to accepted socket list
		if (OK != sock_list_push(&accepted_socks, accepted_socket)) {
			fatal_error(SOCKET_LIST_FAIL);
		}
 	}
}
void respond_accepted()
{
	struct fsm_server_sock_node *sock = NULL;
	int close_sock;
	if (!(sock = accepted_socks.first)) {
		return; 	// no accepted sockets
	}
	do {
		if (0 >= sock->ttl) {
			close_sock = sock->sd;
			sock = sock->next;
			remove_connection(close_sock, &accepted_socks);
			continue;
		}
		if (!(FD_ISSET(sock->sd, &main_set))) {
			sock = sock->next;
			continue;
		}
		// check for partial messages
		if (is_partial(sock->sd, REQ_ST_MES_SIZE)) {
			sock = sock->next;
			continue;
		} 
		close_sock = sock_read_state_req(sock->sd, REQ_ST_MES_SIZE);
		if (close_sock) {
			close_sock = sock->sd;
			sock = sock->next;
			remove_connection(close_sock, &accepted_socks);	// wrong message, close socket
		}
		else {
			// good state req, move to pre-primary
			if (OK != sock_node_move(&accepted_socks, &sr_socks, sock->sd)) {
				fatal_error(SOCKET_LIST_FAIL);
			}
		}
		sock = sock->next;
	} while (sock);
}
void handle_sr_primary()
{
	enum fsm_state_name conv;	// for enum conversion???
	char buffer[4] = {'S', 'R'};
	int sr;
	if (NULL == sr_socks.first || 0 == (sr = sr_socks.first->sd)) {
		return;
	}
	if (!FD_ISSET(sr, &write_set)) {
		if (0 >= sr_socks.first->ttl) {
			remove_connection(sr_socks.first->sd, &sr_socks);
		}
		return;
	}
	// create state response
	if (OK != fsm_current_state_name(&conv)) {
		fatal_error(FSM_FAIL);
	}
	buffer[2] = (char) conv;
	// send response
	if (OK != sock_send_response(sr, buffer, REQ_CH_MES_SIZE)) {
		remove_connection(sr, &sr_socks);	// error sending response
		return;
	}
	// move socket to next state
	if (OK != sock_node_move(&sr_socks, &cs_socks, sr)) {
		fatal_error(SOCKET_LIST_FAIL);
	}
}
void handle_cs_primary()
{
	char req_state;
	int cs;
	if (NULL == cs_socks.first || 0 == (cs = cs_socks.first->sd)) {
		return;
	}
	if (0 >= cs_socks.first->ttl) {
		remove_connection(cs_socks.first->sd, &cs_socks);
		return;
	}
	if (!FD_ISSET(cs, &main_set)) {
		return;
	}
	if (is_partial(cs, REQ_CH_MES_SIZE)) {
		return;
	}
	// read and validate message
	if (OK != sock_read_state_change(cs, &req_state)) {
		remove_connection(cs, &cs_socks);	// wrong message
		return;
	}
	// state change
	if (OK != fsm_switch_state(req_state)) {
		remove_connection(cs, &cs_socks); 	// wrong state was requested
		return;
	}
	// move socket to next state
	if (OK != sock_node_move(&cs_socks, &cr_socks, cs)) {
		fatal_error(SOCKET_LIST_FAIL);
	}
}
void handle_cr_primary()
{
	enum fsm_state_name conv;	// for enum conversion???
	char buffer[4] = {'C', 'R'};
	int cr;
	if (NULL == cr_socks.first || 0 == (cr = cr_socks.first->sd)) {
		return;
	}
	if (!FD_ISSET(cr, &write_set)) {
		if (0 >= cr_socks.first->ttl) {
			remove_connection(cr_socks.first->sd, &cr_socks);
		}
		return;
	}
	// create state response
	if (OK != fsm_current_state_name(&conv)) {
		fatal_error(FSM_FAIL);
	}
	buffer[2] = (char) conv;
	// send response
	if (OK != sock_send_response(cr, buffer, REQ_CH_MES_SIZE)) 
	{
		remove_connection(cr, &cr_socks);	// error sending response
		return;
	}
	// move socket to next state
	if (OK != sock_node_move(&cr_socks, &accepted_socks, cr)) {
		fatal_error(SOCKET_LIST_FAIL);
	}
}
void handle_primary()
{
	handle_sr_primary();
	handle_cs_primary();
	handle_cr_primary();
}
void handle_fsm_logs()
{
	if (!fsm_log_file) {
		return;
	}
	if (FD_ISSET(fsm_log_file, &write_set)) {
		if (OK != fsm_flush_logs()) {
			fatal_error(FSM_FAIL);
		}
	}
}
void add_accepted_socks()
{
	struct fsm_server_sock_node *sock = NULL;
	if ((sock = accepted_socks.first)) {
		do {
			add_fd_to_set(sock->sd, &main_set);
			sock = sock->next;
		} while (sock);
	}
}
void remove_connection(int sock, struct fsm_server_sock_list *list)
{
	if (sock) {
		close(sock);
	}
	if (OK != sock_list_remove(list, sock)) {
		serv_log("remove_connection fatal", 0);
		fatal_error(SOCKET_LIST_FAIL);
	}
}
void add_fd_to_set(int fd, fd_set *set)
{
	if (fd)
	{
		FD_SET(fd, set);
		max_sd = (max_sd > fd) ? max_sd : fd;
	}
}
void serv_log(char *mes, int data)
{
	fprintf(stderr, "[LOG]: <server>: %s, %d\n", mes, data);
}
void fatal_error(enum server_error_name err_name)
{
	fprintf(stderr, "[LOG]: <server>: fatal_error = %d\n", err_name);
	exit(EXIT_FAILURE);
}
void log_server_state()
{
	struct fsm_server_sock_node *node;
	static int counter = 0;
	int accepted_num = 0;
	int sr_num = 0;
	int cs_num = 0;
	int cr_num = 0;
	if (!counter) {
		++counter;
		printf("connections:\n");
		printf("| accepted | sr | cs | cr |\n");
		printf("  %8d, %4d,%4d,%4d", accepted_num, sr_num, cs_num, cr_num);
		fflush(stdout);
	}
	for (node = accepted_socks.first; node != NULL; node = node->next) {
		++accepted_num;
	}
	for (node = sr_socks.first; node != NULL; node = node->next) {
		++sr_num;
	}
	for (node = cs_socks.first; node != NULL; node = node->next) {
		++cs_num;
	}
	for (node = cr_socks.first; node != NULL; node = node->next) {
		++cr_num;
	}
	printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b  %08d, %04d,%04d,%04d", accepted_num, sr_num, cs_num, cr_num);
	fflush(stdout);
}
