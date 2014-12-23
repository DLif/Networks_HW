#ifndef BUFFERED_SOCKET_H
#define BUFFERED_SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include "IO_buffer.h"
#include "nim_protocol_tools.h"


/* 
   this struct represents a buffered socket
   each socket has an input and output buffer
   to buffer protocol messages

   [represents a connection with a client or a server]
*/

typedef struct buffered_socket
{
	int sockfd;                         
	int client_stat;                          /* client status/type: SPECTATOR or PLAYER*/
	struct io_buffer* input_buffer;
	struct io_buffer* output_buffer;
	struct buffered_socket* next_client;      /* in case of a client, list, points to the next client */

} buffered_socket;



/* 
	this method allocates a new buffered socket
	handles and prints errors
	type - in case of a client, holds type of client

	if a malloc error occured, frees resources, prints error and returns NULL
	else, the allocated object is returned 

*/

buffered_socket* create_buff_socket(int sockfd, int status);


/*
	free all resources allocated by a given buffered socket 
*/


void free_buff_socket(buffered_socket* sock);



#endif