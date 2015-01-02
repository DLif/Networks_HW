#ifndef CLIENT_LIST_H
#define CLIENT_LIST_H

#include "buffered_socket.h"
#define MAX_CLIENTS 9

/* represents a list of clients */

typedef struct client_list
{
	struct buffered_socket* first;
	struct buffered_socket* last;
	int size; 

} client_list;



/* initialize a new empty client list */

void init_client_list(client_list* list);

/* add a client to the given list,*/

void add_client(client_list* list, buffered_socket* node);


/*

	method deletes the client from the given client list
	this method will find the buffered_socket of the given client by the the given id and free it (properly)
	if no such client exists, the list won't be changed
*/

void delete_by_client_id(client_list* list, int client_id);


/* 
   add a player to the given list, the addition is done circurality, 
   meaning that the player will be added after all the other existing player have made their move
   the head of the list is given as a parameter, current_player

   [ in order words, the given node will be added right before the current player in the list]
*/
void add_player(client_list* list, buffered_socket* node, int current_player_id);




/*
	this method frees the given list
	the method does not frees the list itself, but rather its content
	it is assumed that list was not allocated using malloc, but rather a local/global variable

	if free_sockets == 1 the socket descriptors will be closed together with the buffers
*/
void free_list(client_list* list, int free_sockets);


/*
	method returns the minimum free client id 
	that will be allocated for a new client connection
*/


int get_minimum_free_client_id(client_list* list);


/**

under the assumption that list contains atleast two players
return the next player id by the order of the list 

*/

int get_next_player_id(client_list* list, buffered_socket* current);




/*
	this method returns a pointer to the buffered_socket of client number req_id
*/
buffered_socket* get_buffered_socket_by_id(int req_id,client_list* client_lst);





/*
	this method promotes a spectator to a player
	modifies the given list as required
	if no spectator is found, returns NULL
	otherwise, returns the spectator that was promoted

	params
		list - the given client list
		current_player_id - id of player that it is currently his turn

*/


buffered_socket* promote_spectator(client_list* list, int current_player_id);



#endif