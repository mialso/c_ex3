#ifndef _MLS_SERVER_SIGNAL_ERROR_H
#define _MLS_SERVER_SIGNAL_ERROR_H

#include "mls_error.h"

static char *error_descriptions[5] = {
	"pipe initialization",
	"signal handling initialization",
	"signal handling",
	"start timer",
	"stop timer"
};
static char *proc_names[5] = {
	"init_pipe",
	"init_signals",
	"timer_signal_handler",
	"!ex!start_timer",
	"!ex!stop_timer"
};
static struct module_error module_err_data = {
	.name = "server_signal",
	.errors = error_descriptions,
	.procs = proc_names
};
static enum module_error_num {
	OK,
	PIPE_INIT_ERR,
	SIGNAL_INIT_ERR,
	TIMER_START,
	TIMER_STOP
}  error_name;
enum module_proc_num {
	INIT_PIPE,
	INIT_SIGNALS,
	TIMER_SIGNAL_HANDLER,
	START_TIMER,
	STOP_TIMER
}  proc_name;

#endif
