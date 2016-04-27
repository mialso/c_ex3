#ifndef _MLS_ERROR_H
#define _MLS_ERROR_H

struct module_error {
	char *name;
	char **errors;
	char **procs;
};

void internal_fatal_error(struct module_error *module_data, int err_num, int proc);
void syscall_fatal_error(struct module_error *module_data, int err_num, int proc);

#endif
