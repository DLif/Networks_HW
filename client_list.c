
#include "client_list.h"



/* initialize a new empty client list */

void init_client_list(client_list* list)
{
	list->size = 0;
	list->first = NULL;
	list->last = NULL;
}

/* add a client to the given list
   use this to simply append a spectator
*/

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
	this method promotes a spectator to a player
	modifies the given list as required
	if no spectator is found, returns NULL
	otherwise, returns the spectator that was promoted

	params
		list - the given client list
		current_player_id - id of player that it is currently his turn
							(put 0 if list is empty )

*/


buffered_socket* promote_spectator(client_list* list, int current_player_id)
{

	/* find the spectator, remove it and re-insert it as a player
	   in the correct place
    */

	buffered_socket* p = list->first;
	buffered_socket* spectator = NULL;

	if( p == NULL)
		return NULL;

	while(p->next_client != NULL)
	{

		if(p->next_client->client_stat == SPECTATOR)
		{
			// found the first spectator
			spectator = p->next_client;

			// now remove it from the list
			// we know for sure that a specator cannot be the head of the list
			// the head of the list, if exists is surely a player
			// he may be however the last item in the list
			if(spectator == list->last)
			{	
				list->last = p;
				p->next_client = NULL;
			}
			else
			{
				p->next_client = spectator->next_client;

			}

			list->size -= 1;
			break;
		}

		// advance
		p = p->next_client;
	}

	if( spectator == NULL)
	{
		// no spectator found
		return NULL;
	}

	spectator->client_stat = PLAYER;

	// else, we found a spectator that we want to promote
	// add it as a player to the correct place
	add_player(list, spectator, current_player_id);

	return spectator;
	
}


/* 
   add a player to the given list, the addition is done circurality, 
   meaning that the player will be added after all the other existing player have made their move
   the head of the list is given as a parameter, current_player

   [ in order words, the given node will be added right before the current player in the list]
*/
void add_player(client_list* list, buffered_socket* node, int current_player_id)
{
	// basicly add the given node right before the current player

	buffered_socket* p = list->first;

	if(p == NULL)
	{
		/* empty list case */
		/* simply append to the end*/
		add_client(list, node);
		return;
	}


	if( p->client_id == current_player_id)
	{
		/* first player in the list */
		/* append to the head of the list */
		node->next_client = p;
		list->first = node;
		list->size += 1;
		return;
		
	}

	while(p->next_client->client_id != current_player_id)
	{
		// advance
		p = p->next_client;
	}

	// we found the node before the the current player
	// insert just between them
	node->next_client = p->next_client;
	p->next_client = node;
	list->size += 1;

	return;

}



/*
	this method find the first spectator in the list, if such exists
	the idea is to promote the first spectator in the list, in other words, the spectator that
	had waited the most

	if all clients are players, a negative value is returned
*/


int get_first_spectator_id(client_list* list)
{
	buffered_socket* p = list->first;

	while(p != NULL)
	{
		if(p->client_stat == SPECTATOR)
			return p->client_id;

		p = p->next_client;
	}
	return -1;

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
	buffered_socket* next, *p    = list->first;
	
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
		next = p->next_client;
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
		p = p->next_client;
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

	buffered_socket* prev = p ;
	p = p->next_client;

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

/*

	method finds a buffered socket from the list by given req_id (should be non negative)
	if no such client is found, NULL is returned 
*/

buffered_socket* get_buffered_socket_by_id(int req_id,client_list* client_lst){

	buffered_socket *running = NULL;
	for(running = client_lst->first; running != NULL; running = running->next_client){
		if (running->client_id == req_id)
		{
			return running;
		}
	}


	return NULL;
}