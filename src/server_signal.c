#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <setjmp.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include "server_signal.h"
#include "server_signal_error.h"

#define MODULE_NAME "<server_signal>"

// global pipe fds
int pipe_fds[2];

// static resources
static struct sigaction sig_act;

// error stuff
static jmp_buf goto_error;
static enum server_signal_error_name module_err;

// service declararions
static void init_pipe(void);
static void init_signals(void);
static inline void pmodule_err(char *mes);
// signal handlers
static void timer_signal_handler();

int init_signal_pipe()
{
	// error handler
	module_err = setjmp(goto_error);
	if (OK < module_err) {
		// handle error
		return module_err;
	}
	// main flow
	init_pipe();
	init_signals();
	
	return OK;
}

void init_pipe()
{
	int flags;
	if (-1 == pipe(pipe_fds)) {
		pmodule_err("init_pipe() pipe failed");
		longjmp(goto_error, PIPE_INIT_ERR);
	}
	// make read end non-blocking
	if (-1 == (flags = fcntl(pipe_fds[0], F_GETFL))) {
		pmodule_err("init_pipe() fcntl F_GETFL failed");
		longjmp(goto_error, PIPE_INIT_ERR);
	}
	flags |= O_NONBLOCK;
	if (-1 == fcntl(pipe_fds[0], F_SETFL, flags)) {
		pmodule_err("init_pipe() fcntl F_SETFL failed");
		longjmp(goto_error, PIPE_INIT_ERR);
	}
	// make write end non-blocking
	if (-1 == (flags = fcntl(pipe_fds[1], F_GETFL))) {
		pmodule_err("init_pipe() fcntl F_GETFL failed");
		longjmp(goto_error, PIPE_INIT_ERR);
	}
	flags |= O_NONBLOCK;
	if (-1 == fcntl(pipe_fds[1], F_SETFL, flags)) {
		pmodule_err("init_pipe() fcntl F_SETFL failed");
		longjmp(goto_error, PIPE_INIT_ERR);
	}

}
void init_signals()
{
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = SA_RESTART;
	sig_act.sa_handler = timer_signal_handler; 	// signal handler
	if (-1 == sigaction(SIGUSR1, &sig_act, NULL)) {
		pmodule_err("init_pipe() sigaction failed");
		longjmp(goto_error, SIGNAL_INIT_ERR);
	}

}
void timer_signal_handler()
{
	int saved_errno;
	
	saved_errno = errno;
	
	if (-1 == write(pipe_fds[1], &timer_signal, 1)) {
		if (EAGAIN != errno) {
			pmodule_err("timer_signal_handler() write failed");
		}
	}
	errno = saved_errno;
}
void pmodule_err(char *mes)
{
	char mes_out[128] = {'\0'};
	size_t prefix_len = strlen(MODULE_NAME);
	size_t mes_len = strlen(mes);
	if (126 < (prefix_len + mes_len)) {
		fprintf(stderr, "[ERROR]: %s module_err message too long\n", MODULE_NAME);
		return;
	}
	if (NULL == strncpy(mes_out, MODULE_NAME, prefix_len)) {
		fprintf(stderr, "[ERROR]: %s module_err strncpy NULL return\n", MODULE_NAME);
		return;
	}
	mes_out[prefix_len-1] = ' ';
	if (NULL == strncpy(mes_out+prefix_len, mes, mes_len)) {
		fprintf(stderr, "[ERROR]: %s module_err strncpy NULL return\n", MODULE_NAME);
		return;
	}
	perror(mes_out);
}

