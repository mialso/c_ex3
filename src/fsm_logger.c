#include <stdio.h>
#include <setjmp.h>
#include <time.h>
#include <stddef.h>
#include <stdlib.h>

#include "fsm_logger.h"
#include "fsm_logger_error.h"

// file definition
static FILE *log_file;
static uint16_t start_time;
static struct fsm_operation current_op;

// fsm-level error
static char *error_message; // max 43 chars

// error handling stuff
jmp_buf do_again;
static enum fsm_logger_error_name logger_error;

// service
static __attribute__ ((noreturn)) void get_log_file();
static uint16_t get_logger_time();
static void print_log();
static void update_operation(int client_id, char type, char state, char error);
static void close_file(void);
static void print_internal_logger_error();


int fsm_log(int client_id, char type, char state, char error)
{
	// error handling, re-run main logic in case of FILE_INIT error
	logger_error = setjmp(do_again);
	if (FILE_INIT < logger_error) {
		print_internal_logger_error();
		return logger_error;
	}
	// main logic, file initialization on first call
	if (NULL != log_file) {
		update_operation(client_id, type, state, error);
		print_log();
		return OK;
	}
	else {
		// init file
		get_log_file();
	}
}
// service definitions
void update_operation(int client_id, char type, char state, char error)
{
	current_op.time = get_logger_time();
	current_op.client_id = client_id;
	current_op.type = type;
	current_op.state = state;
	current_op.error = (error) ? error : ' ';

	// TODO implement error message
	error_message = (error) ? "...not implemented yet..." : "";
}
void print_log()
{
	int length = 0;
	if (64 == (length = fprintf(
				log_file, 
				"%05d %05d %c %c %c <%-43s>\n", 
				current_op.time, 
				current_op.client_id, 
				current_op.type, 
				current_op.state, 
				current_op.error, 
				error_message))) 
	{
		// do nothing realy
	}
	else {
		longjmp(do_again, OUTPUT_ERROR);
	}
}

void get_log_file()
{
	static int init_attempt = FILE_INIT;
	int result = 0;
	if (NULL == (log_file = fopen("fsm.log", "w"))) {
		result = FILE_ERROR;
	}
	else {
		// file initialization
		atexit(close_file);
		result = init_attempt++; // only one init allowed, to prevent infinite loop in enormous case
		start_time = get_logger_time();
	}
	// jump to handler with error code
	longjmp(do_again, result);
}
uint16_t get_logger_time()
{
	uint16_t result = 0;
	time_t current_time;

	if (((time_t) (-1)) == (current_time = time(NULL))) {
		// error condition, unable to get time
		longjmp(do_again, CLOCK_ERROR);
	}
	else {
		// normal flow
		result = (uint16_t) current_time;
		if (start_time) {
			result -= start_time;
		}
	}
	return result;
}
void print_internal_logger_error()
{
	fprintf(stderr, "[ERROR]: <fsm_logger>: %d\n", logger_error);
}
void close_file(void)
{
	fclose(log_file);
}
