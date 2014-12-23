#ifndef CLIENT_LIST_H
#define CLIENT_LIST_H

#include "buffered_socket.h"


/* represents a list of clients */

typedef struct client_list
{
	struct buffered_socket* first;
	struct buffered_socket* last;
	int size; 

} client_list;



/* initialize a new empty client list */

void init_client_list(client_list* list);

/* add a client to the given list */

void add_client(client_list* list, buffered_socket* node);


/*

	method deletes the client from the given client list
	this method will find the buffered_socket of the given client by the the given id and free it (properly)
	if no such client exists, the list won't be changed
*/

void delete_by_client_id(client_list* list, int client_id);

#endif