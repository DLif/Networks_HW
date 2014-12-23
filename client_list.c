
#include "client_list.h"



/* initialize a new empty client list */

void init_client_list(client_list* list)
{
	list->size = 0;
	list->first = NULL;
	list->last = NULL;
}

/* add a client to the given list */

void add_client(client_list* list, buffered_socket* node)
{

	list->size += 1;

	// in case of an empty list
	if(list->size == 0)
	{
		list->first = list->last = node;
		return;

	}

	// append to the end
	list->last->next_client = node;
	list->last = node;


}