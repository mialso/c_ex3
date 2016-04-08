#include <stdio.h>
#include <stddef.h>
#include "server_socket_list.h"

#define NODE_SIZE sizeof(struct fsm_server_sock_node)

// service declarations
static int remove_node(struct fsm_server_sock_list *list, struct fsm_server_sock_node *node);

int sock_list_push(struct fsm_server_sock_list *list, int sd)
{
	struct fsm_server_sock_node *node;
	if (list && sd) {
		// TODO implement find as a function
		//check the node is in list
		if ((node = list->first)) {
			do {
				if (sd == node->sd) {
					return 7;	// is already in list 
				}
				node = node->next;
			} while (node);
			node = NULL;
		}
		node = calloc(1, NODE_SIZE);
		
		if (node) {
			// main logic
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
			//++(list->count);
			//list->count++;
			return 0;
		}
		else {
			fprintf(stderr, "[LOG]: fsm_server_socket: memory error\n");
			return 1;	// memory error
		}
	}
	else {
		fprintf(stderr, "[LOG]: fsm_server_socket: args error\n");
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
				remove_node(list, current_node);
				return 0; 	// OK
			}
			else {
				current_node = current_node->next;
			}
		} while (current_node);
		return 3; 	// no node with such sd value
	}
	else {
		if (list->last) {
			return 6; 	// corrupted list
		}
		else {
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
	else {
		return 5; 	// empty list
	}
}
int remove_node(struct fsm_server_sock_list *list, struct fsm_server_sock_node *node)
{
	// check args
	if ((!node) || (!list)) {
		return 2; 		// wrong args
	}
	// check list
	if ((!list->first)) {
		if (!list->last) {
			return 5;	// empty list
		}
		else {
			return 6;	// corrupted list
		}
	}
	// first node logic
	if (!(node->prev) && (list->first == node)) {
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
