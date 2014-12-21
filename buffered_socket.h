#ifndef IO_BUFFER_H
#define IO_BUFFER_H
#include "IO_buffer.h"


#define PLAYER 1
#define SPECTATOR 0

/* 
   this struct represents a buffered socket
   each socket has an input and output buffer
   to buffer protocol messages
*/

typedef struct buffered_socket
{
	int sockfd; 
	int client_stat;
	io_buffer* input_buffer;
	io_buffer* output_buffer;

} buffered_socket;

/* 
	this method allocates a new buffered socket
	handles and prints errors

	if a malloc error occured, frees resources, prints error and returns NULL
	else, the allocated object is returned 

*/

buffered_socket* create_buff_socket(int sockfd,int client_new_stat);


/*
	free all resources allocated by a given buffered socket 
*/


void free_buff_socket(buffered_socket* sock);



#endif