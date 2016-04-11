#include <stdio.h>
#include <setjmp.h>
#include <time.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fsm_logger.h"
#include "fsm_logger_error.h"

#define LOG_FILE_PATH "./bin/fsm.log"

// static definitions
static uint16_t start_time;
static struct fsm_operation *first_op;

// fsm-level error
static char *error_message; // max 43 chars

// error handling stuff
jmp_buf do_again;
static enum fsm_logger_error_name logger_error;

// service
static __attribute__ ((noreturn)) void get_log_file();
static uint16_t get_logger_time(void);
static void update_operation(int client_id, char type, char state, char error);
static void close_file(void);
static void print_internal_logger_error(enum fsm_logger_error_name err);
static struct fsm_operation *flush_one(struct fsm_operation *op, int *res);


int fsm_log(int client_id, char type, char state, char error)
{
	// error handling, re-run main logic in case of FILE_INIT error
	logger_error = setjmp(do_again);
	if (FILE_INIT < logger_error) {
		print_internal_logger_error(logger_error);
		return logger_error;
	}
	// main logic, file initialization on first call
	if (fsm_log_file) {
		update_operation(client_id, type, state, error);
		return OK;
	}
	else {
		// init file
		get_log_file();
	}
}
int fsm_logger_flush_logs()
{
	int res;
	struct fsm_operation *next_op;
	if (!first_op) {
		return OK; 	// nothing to do
	}
	next_op = first_op;
	while ((next_op = flush_one(next_op, &res))) {
		continue;
	}
	
	return 0;
}
// service definitions
struct fsm_operation *flush_one(struct fsm_operation *op, int *res)
{
	char buf[64] = {'\0'};
	struct fsm_operation *tmp;
	if (64 != (*res = sprintf(
				buf, 
				"%05d %05d %c %c %c <%-43s>\n", 
				op->time, 
				op->client_id, 
				op->type, 
				op->state, 
				op->error, 
				error_message))) 
	{
		print_internal_logger_error(CONVERT_ERROR);
		return NULL;
	}
	if (-1 == (*res = write(fsm_log_file, buf, 64))) {
		perror("fsm_logger: fsm_logger_flush_logs(): flush_one write error");
		return NULL;
	}
	if (64 != *res) {
		print_internal_logger_error(OUTPUT_ERROR);
		return NULL;
	}
	tmp = op->next;
	first_op = tmp;
	free(op); 
	return tmp;
}
void update_operation(int client_id, char type, char state, char error)
{
	struct fsm_operation *last_op;
	struct fsm_operation *op = calloc(1, sizeof (struct fsm_operation));
	if (!op) {
		perror("fsm_logger: update_operation() calloc error");
		longjmp(do_again, MEMORY_ERROR);
	}
	op->next = NULL;
	op->prev = NULL;
	op->time = get_logger_time();
	op->client_id = client_id;
	op->type = type;
	op->state = state;
	op->error = (error) ? error : ' ';
	
	if (first_op) {
		// go to last operation
		last_op = first_op;
		while (last_op->next) {
			last_op = last_op->next;
		}
		op->prev = last_op;
		last_op->next = op;
	}
	else {
		first_op = op;
	}
	
	// TODO implement error message
	error_message = (error != ' ') ? "...not implemented yet..." : "";
}

void get_log_file()
{
	static int init_attempt = FILE_INIT;
	int result = 0;
	if (-1 == (fsm_log_file = open(LOG_FILE_PATH, O_CREAT | O_WRONLY | O_TRUNC | O_NONBLOCK, S_IRUSR | S_IWUSR))) {
		result = FILE_ERROR;
		perror("fsm_logger: get_log_file() open error");
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
void print_internal_logger_error(enum fsm_logger_error_name err)
{
	logger_error = err;
	fprintf(stderr, "[ERROR]: <fsm_logger>: %d\n", logger_error);
}
void close_file(void)
{
	close(fsm_log_file);
}
