#ifndef IO_BUFFER_H
#define IO_BUFFER_H
#include "nim_protocol_tools.h"
#include "socket_IO_tools.h"

#define MAX_IO_BUFFER_SIZE 2000   /* maximum size of input/output buffer */
#define OVERFLOW_ERROR     1
#define INVALID_MESSAGE    2
#define MSG_NOT_COMPLETE   3

/*
	this struct represents a FIFO circular buffer for buffered reading or writing to/from a socket
*/

typedef struct io_buffer
{
	int size;                         /* number of items currently in buffer */
	int head;                         /* index of the first byte */
	int tail;                         /* index of the last byte */
	char buffer[MAX_IO_BUFFER_SIZE];  /* the actual buffer */

} io_buffer;


/*
	method pushes num_bytes from source_buffer to given io_buffer struct
	returns OVERFLOW_ERROR if buffer cannot push num_bytes (positive value)
	otherwise returns 0
*/

int push(io_buffer* buff, char* source_buffer, int num_bytes);

/*
	this method pops num_bytes from buff struct (circular buffer)
	and puts them in target_buffer
	the method returns 1 on error (num_bytes too big)

	or 0 on success
*/

int pop(io_buffer* buff, char* target_buffer, int num_bytes);


/* same as previous, no target buffer */
int pop_no_return(io_buffer* buff, int num_bytes);


/*

	this method pops a message from the given buff struct, puts the result in msg_container
	if an illegal message is found in the buffers head, returns INVALID_MESSAGE (invalid according to the protocol)
	if only a part of a legal message is found, returns MSG_NOT_COMPLETE
	otherwise, pops a whole legal message header from the buff, updates it accordingly
	and puts the result in msg_container, in this case SUCCESS is returned (0)

	special case:
		if the message to be popped is of type MSG (peer to peer message), 
		the method checks that the message also exists on the buffer,
		if so, the buffer is popped and the header (not the message itself) is returned

		otherwise, if a part of the message is missing, returns MSG_NOT_COMPLETE
*/


int pop_message(io_buffer* buff, message_container* msg_container);



/*
	same as send_partially as defined in socket_IO_tools
	but handles sending from a circular buffers

	returns a negative value on error (malloc error may also occur)
*/
int send_partially_from_buffer(io_buffer* buff, int sockfd, int num_bytes, int* connection_closed);

#endif 