#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include "server_signal.h"
#include "server_signal_error.h"
#include "server_signal_timer.h"

#define TIMER_INTERVAL 200
// pipe ends marked explicitly
#define READ_END 0
#define WRITE_END 1

// global pipe fds
int pipe_read_fd;

// static resources
static struct sigaction sig_act;
static int pipe_fds[2];

// service declararions
static void init_pipe(void);
static void init_signals(void);
// signal handlers
static void timer_signal_handler();

void init_signal_pipe()
{
	int timer_error;
	// main flow
	init_pipe();
	init_signals();
	if ((timer_error = start_timer(TIMER_INTERVAL))) {
		proc_name = START_TIMER;
		syscall_fatal_error(&module_err_data, timer_error, proc_name);
	}
	if ((timer_error = atexit(&stop_timer))) {
		proc_name = STOP_TIMER;
		error_name = STOP_TIMER;
		internal_fatal_error(&module_err_data, error_name, proc_name);
	}
	
	pipe_read_fd = pipe_fds[READ_END];
}

void init_pipe()
{
	int flags;
	proc_name = INIT_PIPE;
	if (-1 == pipe(pipe_fds)) {
		syscall_fatal_error(&module_err_data, errno, proc_name);
	}
	// make read end non-blocking
	if (-1 == (flags = fcntl(pipe_fds[READ_END], F_GETFL))) {
		syscall_fatal_error(&module_err_data, errno, proc_name);
	}
	flags |= O_NONBLOCK;
	if (-1 == fcntl(pipe_fds[READ_END], F_SETFL, flags)) {
		syscall_fatal_error(&module_err_data, errno, proc_name);
	}
	// make write end non-blocking
	if (-1 == (flags = fcntl(pipe_fds[WRITE_END], F_GETFL))) {
		syscall_fatal_error(&module_err_data, errno, proc_name);
	}
	flags |= O_NONBLOCK;
	if (-1 == fcntl(pipe_fds[WRITE_END], F_SETFL, flags)) {
		syscall_fatal_error(&module_err_data, errno, proc_name);
	}

}
void init_signals()
{
	proc_name = INIT_SIGNALS;

	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = SA_RESTART;
	sig_act.sa_handler = timer_signal_handler; 	// signal handler
	if (-1 == sigaction(SIGUSR1, &sig_act, NULL)) {
		syscall_fatal_error(&module_err_data, errno, proc_name);
	}

}
void timer_signal_handler()
{
	proc_name = TIMER_SIGNAL_HANDLER;
	int saved_errno;
	
	saved_errno = errno;
	if (-1 == write(pipe_fds[WRITE_END], &timer_signal, 1))
	{
		if (EAGAIN != errno) {
			syscall_fatal_error(&module_err_data, errno, proc_name);
		}
	}
	errno = saved_errno;
}

