#include <sys/types.h>
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

#define NETWORK_FUNC_FAILURE -1 /* error code */
#define AWATING_CLIENTS_NUM   1 /* connection queue size */

/*  this method checks whether the given arguments given in command line are valid
	returns true if are valid, returns false otherwise
*/
bool checkServerArgsValidity(int argc ,const char* args[]);

/* method allocates a socket fd for listening to incoming connections on given port number
On success returns the listening socket fd. In case of failure returns NETWORK_FUNC_FAILURE constant 
*/
int initServer(short port);


/*
	this method sends a message to the client containing the following information:
	flags: last_time_validty (was the last move by client valid)
		   game over (victor contains the identity of the winner, if any)
    sizes of each heap (four shorts)

	input: socket - connection with client
		   last_time_validity - true iff last move by client was valid
		   victor - holds SERVER, CLIENT or NONE
	returns true on error, returns false on success
*/
bool sendResultsOfRound(int socket, bool last_time_validity, int victor);

/* makes and sends openning message to the client, contains: isMisere value and the heaps' sizes
input: isMisere - should be according to the game type (1 iff game is of type misere )
        socket - the socket with which we send the data to the client
returns false if successeded, else true
*/
bool sendGameDetails(int socket,bool isMisere);

/*
	this method recieves the query from the client and fills the given parameters
	*player_heapNum will hold the heap index to remove from (starting from 0), *player_size will hold how many items the player
	wants to remove.
	socket is the socket with which we communicate the data to the client
	returns false if successeded, else true
*/
bool receiveAndParse(int socket,char* player_heapNum,short* player_size);

/* 
   this method sends given message to the client via sockfd
   returns true iff error occured 
*/
bool send_server_message(int sockfd, char* buff, int num_bytes);