#ifndef _SERVER_UTILS
#define _SERVER_UTILS

/*
	this module provides event handlers for the server and various helper methods to send and recieve data
*/


#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "buffered_socket.h"
#include "nim_protocol_tools.h"
#include "IO_buffer.h"
#include "nim_game.h"
#include "socket_IO_tools.h"
#include "client_list.h"

#define ERROR -1      /* error code */
#define END_GAME 5    /* error code: value to indicate game has ended */

/* list of client, each client is represented by a buffered socket struct */

extern client_list clients_linked_list;

/* whose turn is it (holds the client id of the player that should move now )

   */
extern int current_turn; 

/*
	how many players are there currently 
*/

extern int current_player_num;

/*
	p value, how many players are allowed 
*/

extern int max_players_allowed;



/*

	this method pops whole messages, and handles
	valid messages may be of types MSG (peer to peer message)
	or PLAYER_MOVE_MSG ( some player wishes to make a move )

	if some error occured, a proper message will be printed, and ERROR returned
	if game successfully ended, returns END_GAME
	o/w 0 is returned

*/

int handle_ready_messages();





/* find the next player to play */
void update_turn();




/*
	this method handles a client to client message request
	method also handles errors (is the message actually valid, output buffers overflow)

*/

int chat_message_handle(int sender_id, client_to_client_message *message);


/* 
	handles a game move message from client
	returns ERROR if some error occured
	returns END_GAME if game successfully ended 
*/


int game_message_handle(int client_id, player_move_msg* message);


/*
	method handles a client exit
	since it may be a player, special treatment is needed
	in case of a player, if there are spectators waiting, promote the spectator that connected first
	check if the player that disconnected was suppose to make a move and pass the turn to the next player
*/

int quit_client_handle(int quiting_client);


/*
	method notifies next_player that it is his move
*/

int send_your_move();

// method initializes the given read set
// adds the listening socket and all client sockets

void setReadSet(fd_set* read_set, int listeningSoc);


// method intiializes the given write set
// adds all the client sockets in it
void setWriteSet(fd_set* write_set);



//this function will find the max socket number. used for select
int findMax(int listeningSoc);

/*

	method sends the output buffers (does not use sendall, sends how much is premitted by send 
	to the client sockets
	if a connection is closed suddenly it is simply dropped and an error message will be printed
*/

void send_info(fd_set* write_set);




/*

	method "sends" a heap update message to all the clients
	(pushes on output buffers)
	on error, returns ERROR o/w returns 0
*/

int send_heaps_update(int game_over);



/*

	method handles sending ACK or NACK to current player, according to is_legal_move
	
	returns ERROR on error, or 0 if successfull

*/
	
int send_move_ACK(int current_client, bool is_legal_move);


/*

	iterate over the clients in the read_set, and receive as much as you can from each client socket
	(fill input buffers)
	on any error, a proper message is printed and ERROR is returned
	on success, 0 is returned

	NOTE: this method also handles closed connections
*/
int read_to_buffs(fd_set* read_set);


/*

	after the game has ended, this method
	tries to send the information (using select) untill all players either closed connection or all data was sent

	returns ERROR on error, 0 otherwise 
*/


int send_final_data();



#endif