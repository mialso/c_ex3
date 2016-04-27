#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "mls_error.h"

#define MAX_MES_OUT 128
#define PREF "[ERROR]:"

void syscall_fatal_error(struct module_error *module_data, int err_num, int proc)
{
	fprintf(stderr, "%s <%s>: %s()\n", PREF, module_data->name, module_data->procs[proc]);
	errno = err_num;
	perror("syscall error");
	exit(EXIT_FAILURE);
}
void internal_fatal_error(struct module_error *module_data, int err_num, int proc)
{
	fprintf(stderr, "%s <%s>: %s(): %s\n", PREF, module_data->name, module_data->procs[proc], module_data->errors[err_num]);
	exit(EXIT_FAILURE);
}
