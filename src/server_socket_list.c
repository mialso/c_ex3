#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "server_socket_list.h"

#define NODE_SIZE sizeof(struct fsm_server_sock_node)
#define MODULE_NAME "<server_socket_list>"

// service declarations
static int remove_node(struct fsm_server_sock_list *list, struct fsm_server_sock_node *node);
static void pmodule_err(char *mes, int is_sys_err);

int sock_list_push(struct fsm_server_sock_list *list, int sd)
{
	struct fsm_server_sock_node *node;
	if (list && sd) {
		// TODO implement find as a function
		//check the node is in list
		if ((node = list->first)) {
			do {
				if (sd == node->sd) {
					pmodule_err("sock_list_push(): is already in list", 0);
					return 7;	// is already in list 
				}
				node = node->next;
			} while (node);
			node = NULL;
		}
		node = calloc(1, NODE_SIZE);
		
		if (node) {
			// main logic
			node->ttl = list->ttl;
			node->sd = sd;
			if (NULL == list->last) {
				list->last = node;
				list->first = node;
			}
			else {
				list->last->next = node;
				node->prev = list->last;
				list->last = node;
			}
			return 0;
		}
		else {
			pmodule_err("sock_list_push(): memory error", 1);
			return 1;	// memory error
		}
	}
	else {
		pmodule_err("sock_list_push(): args error", 0);
		return 2; 		// wrong args supplied
	}
}
int sock_list_remove(struct fsm_server_sock_list *list, int sd)
{
	struct fsm_server_sock_node *current_node = list->first;
	if (current_node) {
		do {
			// that is the node to remove
			if (sd == current_node->sd) {
				if (0 != remove_node(list, current_node)) {
					pmodule_err("sock_list_remove(): remove_node() error", 0);
					return 9;
				}
				return 0; 	// OK
			}
			else {
				current_node = current_node->next;
			}
		} while (current_node);
		pmodule_err("sock_list_remove(): no node with such sd value", 0);
		return 3; 	// no node with such sd value
	}
	else {
		if (list->last) {
			pmodule_err("sock_list_remove(): corrupted list", 0);
			return 6; 	// corrupted list
		}
		else {
			pmodule_err("sock_list_remove(): empty list", 0);
			return 4; 	// empty list
		}
	}
}
int sock_node_move(struct fsm_server_sock_list * restrict l_from, 
			struct fsm_server_sock_list * restrict l_to, int sd)
{
	int res;
	// check remove error
	if ((res = sock_list_remove(l_from, sd))) {
		return res;
	}
	if ((res = sock_list_push(l_to, sd))) {
		return res;
	}
	return 0;
}
int sock_list_pop(struct fsm_server_sock_list *list, int *sd)
{
	if (list->last) {
		*sd = list->last->sd;
		return 0; 	// OK
	}
	pmodule_err("sock_list_pop(): empty list", 0);
	return 5; 	// empty list
}
int sock_list_update_ttl(struct fsm_server_sock_list *list)
{
	struct fsm_server_sock_node *node;
	for (node = list->first; node != NULL; node = node->next) {
		--(node->ttl);
	}
	return 0;
}
int remove_node(struct fsm_server_sock_list *list, struct fsm_server_sock_node *node)
{
	// check args
	if (!node || !list) {
		return 2; 		// wrong args
	}
	// check list
	if (!list->first) {
		if (!list->last) {
			return 5;	// empty list
		}
		else {
			return 6;	// corrupted list
		}
	}
	// first node logic
	if (list->first == node && !node->prev) {
		// the only one node logic
		if (!(node->next) && (list->last == node)) {
			list->first = NULL;
			list->last = NULL;
		}
		else {
			list->first = node->next;
			list->first->prev = NULL;
		}
	}
	// last node logic
	else if (!(node->next) && (list->last == node)) {
		list->last = node->prev;
		list->last->next = NULL;
	}
	// center node logic
	else if (node->prev && node->next) {
		if ((list->last == node) || (list->first == node)) {
			return 6;
		}
		node->next->prev = node->prev;
		node->prev->next = node->next;
	}
	else {
		return 6;	// corrupted list
	}

	free(node);

	return 0; 	// OK
}
void pmodule_err(char *mes, int is_sys_err)
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
	if (is_sys_err) {
		perror(mes_out);
	}
}
