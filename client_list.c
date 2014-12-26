
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
	this method finds the minimum spectator id in the given list
	returns MAX_CLIENTS + 1 if no spectators found
*/

int get_min_spectator_id(client_list* list)
{

	int min; 
	
	buffered_socket* p    = list->first;

	min = MAX_CLIENTS + 1;

	while( p != NULL)
	{
		if(p->client_stat == SPECTATOR)
		{	
			min = p->client_id < min ? p->client_id : min;
		}
		p = p->next_client;
	}

	return min;


}


/*
	this method frees the given list
	the method does not frees the list itself, but rather its content
	it is assumed that list was not allocated using malloc, but rather a local/global variable

	parameter free_sockets should hold 1 if the user wishes to close all the socket descriptors
	0 otherwise

*/
void free_list(client_list* list, int free_sockets)
{
	buffered_socket* p    = list->first;
	buffered_socket* next = p->next_client; /* maintain next! */
	
	if(free_sockets)
	{
		// close all file descriptors

		while(p != NULL)
		{
			close_socket(p->sockfd);
			p = p->next_client;
		}
		
		// return to first node
		p = list->first;
		
	}
	

	while( p != NULL)
	{
		/* this frees p itself as well */
		free_buff_socket(p);
		p = next;
	}
}





/*
	method returns the minimum free client id 
	that will be allocated for a new client connection
*/

int get_minimum_free_client_id(client_list* list)
{
	if(list->size == 0)
		/* ids start with 1 */
		return 1;

	int seen[MAX_CLIENTS] = { 0 };

	buffered_socket* p    = list->first;
	
	while( p != NULL)
	{
		seen[p->client_id - 1] = 1;
		p = next;
	}

	int i ;
	for(i = 0; i < MAX_CLIENTS; ++i)
	{
		if(!seen[i])
		{
			// this is the minimum id that was not found
			return i+1;
		}
	}

	return -1;
	
}


/**

under the assumption that list contains atleast one player
return the next player id by the order of the list (including current )

*/

int get_next_player_id(client_list* list, buffered_socket* current)
{
	
	buffered_socket* p = current;

	if(p == NULL)
	{
		p = list->first;
	}

	while(p->client_stat != PLAYER)
	{
		p = p->next_client;
		if(p == NULL)
		{
			p = list->first;
		}
	}

	// found a player

	return p->client_id;

}




/*

	method deletes the client from the given client list
	this method will find the buffered_socket of the given client by the the given id and free it (properly)
	if no such client exists, the list won't be changed
	the method will also close the socket!
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
		close_socket(p->sockfd);
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

			close_socket(p->sockfd);
			free_buff_socket(p);
			list->size -= 1;
			return;
		}

		// move to next node, update previous node
		prev = p;
		p = p->next_client;

	}

}