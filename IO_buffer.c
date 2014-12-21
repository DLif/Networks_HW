#include "IO_buffer.h"
#include "nim_protocol_tools.h"

/*
	method pushes num_bytes from source_buffer to given io_buffer struct
	returns OVERFLOW_ERROR if buffer cannot push num_bytes (positive value)
	otherwise returns 0
*/

int push(io_buffer* buff, char* source_buffer, int num_bytes)
{

	// check buffer overflow
	if(buff->size + num_bytes > MAX_IO_BUFFER_SIZE)
	{
	
		return OVERFLOW_ERROR;
	}

	int i;
	int start_index = (buff->tail + 1) % MAX_IO_BUFFER_SIZE;
	
	// push data to buffer

	for( i = 0; i < num_bytes; ++i)
	{
		buff->buffer[(start_index + i) % MAX_IO_BUFFER_SIZE] = source_buffer[i];
	}

	// update fields
	buff->size += num_bytes;
	buff->tail = (buff->tail + num_bytes) % MAX_IO_BUFFER_SIZE;

	// return success
	return 0;

}

/*
	this method pops num_bytes from buff struct (circular buffer)
	and puts them in target_buffer
	the method returns 1 on error (num_bytes too big)

	or 0 on success
*/

int pop(io_buffer* buff, char* target_buffer, int num_bytes)
{

	if(num_bytes > buff->size)
		return 1;

	int i ;

	// fill the target buffer
	for( i = 0; i < num_bytes; ++i)
	{
		target_buffer[i] = buff->buffer[(buff->head + i) % MAX_IO_BUFFER_SIZE];
	}

	// update invariants
	buff->head = (buff->head + num_bytes) % MAX_IO_BUFFER_SIZE;
	buff->size -= num_bytes;

	return 0;

}



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


int pop_message(io_buffer* buff, message_container* msg_container)
{
	/* read the first byte first, that defines the type of the message */
	/* the first byte is is at index head                              */
	char message_type = buff->buffer[buff->head];
	msg_container->message_type = message_type;

	int message_size = get_message_size(message_type);  /* get the size of the actual struct    */

	if(message_size < 0)
	{
		/* invalid message */
		return INVALID_MESSAGE;
	}

	if(message_size == 1)
	{
		/* we only needed to read a single byte, that defines the message */
		return SUCCESS;
	}

	/* check if we can pop the header */
	if(buff->size >= message_size)
	{
		if(message_type == MSG) /* client to client message */
		{
			client_to_client_message msg = (client_to_client_message*)(buff->buffer);

			/* we also need to check that the actual message is on the buffer */
			if(buff->size >= message_size + msg->length)
			{
				// the actual message is in the buffer
				// pop the header and store it in msg_container
				pop(buff, msg_container, message_size);
				if(valiadate_message(msg_container))
				{
					return INVALID_MESSAGE;
				}

				return SUCCESS;
			}
			else
			{
				// the message itself is not on the buffer yet, gotta wait
				// do not pop anything
				return MSG_NOT_COMPLETE;
			}

		}
		else
		{
			// pop the message 
			pop(buff, msg_container, message_size);
			if(valiadate_message(msg_container))
			{
				return INVALID_MESSAGE;
			}
			return SUCCESS;
		}


	}

	/* otherwise, only a part of the message is on the buffer, wait for the rest */

	return MSG_NOT_COMPLETE;

}
