#include <stdio.h>
#include <stdlib.h>

#include "../src/mls_error.h"

char *name = "module 1";
enum errs {
	ERR1,
	ERR2,
	ERR3
};
char *errors[3] = {
	"error 1",
	"error 2",
	"error 3"
};
char *procs[3] = {
	"proc 1",
	"proc 2",
	"proc 3"
};
struct module_error m_e;
int fail;

void test_internal_fatal_error();
void test_syscall_fatal_error();
void exit_syscall_handler(void);
void exit_internal_handler(void);

int main(int argc, char *argv[])
{
	m_e.name = name;
	m_e.errors = errors;
	m_e.procs = procs;

	if (argc < 2)
		return 0;

	if ('s' == argv[1][0]) {
		test_syscall_fatal_error();
	}
	if ('i' == argv[1][0]) {
		test_internal_fatal_error();
	}

	return 0;
}

void test_syscall_fatal_error()
{
	if ((atexit(exit_syscall_handler))) {
		printf("[ERROR]: <test_syscall_fatal_error>: test_syscall_fatal_error()\n");
	}
	syscall_fatal_error(&m_e, 2, 1);
	fail = 1;
	
}
void test_internal_fatal_error()
{
	if ((atexit(exit_internal_handler))) {
		printf("[ERROR]: <test_internal_fatal_error>: test_internal_fatal_error()\n");
	}
	internal_fatal_error(&m_e, ERR2, 1);
	fail = 1;
	
}
void exit_syscall_handler(void)
{
	if (fail) {
		printf("[FAILED]: test_syscall_fatal_error\n");
		return;
	}
	printf("[PASSED]: test_syscall_fatal_error\n");
}
void exit_internal_handler(void)
{
	if (fail) {
		printf("[FAILED]: test_internal_fatal_error\n");
		return;
	}
	printf("[PASSED]: test_internal_fatal_error\n");
}


