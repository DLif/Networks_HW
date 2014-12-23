
#include "IO_buffer.h"
#include <malloc.h>
#include <stdio.h>

/* 
   this struct represents a buffered socket
   each socket has an input and output buffer
   to buffer protocol messages
*/

typedef struct buffered_socket
{
	int sockfd; 
	io_buffer* input_buffer;
	io_buffer* output_buffer;

} buffered_socket;

/* 
	this method allocates a new buffered socket
	handles and prints errors

	if a malloc error occured, frees resources, prints error and returns NULL
	else, the allocated object is returned 

*/

buffered_socket* create_buff_socket(int sockfd);


/*
	free all resources allocated by a given buffered socket 
*/


void free_buff_socket(buffered_socket* sock);


