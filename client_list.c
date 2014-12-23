
#include "client_list.h"



/* initialize a new empty client list */

void init_client_list(client_list* list)
{
	list->size = 0;
	list->first = NULL;
	list->last = NULL;
}

/* add a client to the given list (at the end)*/

void add_client(client_list* list, buffered_socket* node)
{

	list->size += 1;

	// in case of an empty list
	if(list->size == 1)
	{
		list->first = node;
		list->last = node;
		return;

	}

	// append to the end
	list->last->next_client = node;
	list->last = node;


}


/*

	method deletes the client from the given client list
	this method will find the buffered_socket of the given client by the the given id and free it (properly)
	if no such client exists, the list won't be changed
*/

void delete_by_client_id(client_list* list, int client_id)
{


	buffered_socket* p = list->first;


	if(p == NULL)
	{
		return;
	}

	if(p->client_id == client_id)
	{
		// first is the desired node
		// update list's first
		list->first = p->next_client;

		if(list->size == 1)
		{
			// p is also the last node
			list->last = NULL;
		}

		// delete the client node
		free_buff_socket(p);

		list->size -= 1;


		return ;
	}


	p = p->next_client;
	buffered_socket* prev = p ;

	while(p != NULL)
	{
		if(p->client_id == client_id)
		{
			// delete
			prev->next_client = p->next_client;
			
			if(list->last == p)
			{
				// p is also the last
				list->last = prev;
			}

			free_buff_socket(p);
			list->size -= 1;
			return;
		}

		// move to next node, update previous node
		prev = p;
		p = p->next_client;

	}

}