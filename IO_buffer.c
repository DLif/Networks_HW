#include "IO_buffer.h"
#include <stdio.h>


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
	int start_index = (buff->tail) % MAX_IO_BUFFER_SIZE;//was buff->tail+1
	
	//printf("%d \n", start_index);
	// push data to buffer

	for( i = 0; i < num_bytes; ++i)
	{
		//printf("source_buffer %d : %d\n",i, source_buffer[i]);
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
	//printf("buff->head + i %d \n",buff->head + 0 );
	//printf("buff->buffer[(buff->head + i) mod MAX_IO_BUFFER_SIZE] %d \n",buff->buffer[(buff->head + 0) % MAX_IO_BUFFER_SIZE] );
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
	this method pops num_bytes from buff struct (circular buffer)
	the method returns 1 on error (num_bytes too big)

	or 0 on success
*/

int pop_no_return(io_buffer* buff, int num_bytes)
{

	if(num_bytes > buff->size)
		return 1;

	// update invariants
	buff->head = (buff->head + num_bytes) % MAX_IO_BUFFER_SIZE;
	buff->size -= num_bytes;

	return 0;

}

/*
	same as send_partially as defined in socket_IO_tools
	but handles sending from a circular buffers

	returns a negative value on error (malloc error may also occur)
*/

int send_partially_from_buffer(io_buffer* buff, int sockfd, int num_bytes, int* connection_closed)
{
	char* temp_buff = (char*)malloc(num_bytes*sizeof(char));

	if(temp_buff == NULL)
	{
		*connection_closed = 0;
		return -1;

	}

	// fill temp_buffer

	int head = buff->head;

	int i;
	for(i = 0; i < num_bytes; ++i)
	{
		temp_buff[i] = buff->buffer[(head+i) % MAX_IO_BUFFER_SIZE];
	}

	int res = send_partially(sockfd, temp_buff, num_bytes, closed_connection);
	free(temp_buff);

	return res;


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
	//if buffer empty, can't pop message(no message exists).
	if (buff->size == 0)
	{
		//no messages in buffer
		return MSG_NOT_COMPLETE;
	}
	/* read the first byte first, that defines the type of the message */
	/* the first byte is is at index head                              */
	char message_type = buff->buffer[buff->head];

	int message_size = get_message_size(message_type);  /* get the size of the actual struct    */
	if(message_size < 0)
	{
		/* invalid message */
		return INVALID_MESSAGE;
	}

	/* check if we can pop the header */
	if(buff->size >= message_size)
	{
		if(message_type == MSG) /* client to client message */
		{
			client_to_client_message* msg = (client_to_client_message*)(buff->buffer);

			/* we also need to check that the actual message is on the buffer */
			if(buff->size >= message_size + (unsigned char)(msg->length))
			{
				// the actual message is in the buffer
				// pop the header and store it in msg_container
				pop(buff, (char*)msg_container, message_size);
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
			pop(buff, (char*)msg_container, message_size);

			if(message_size == 1)
			{
				// message was already validated by get_message_size
				// we popped what we wanted
				return SUCCESS;
			}

			if(valiadate_message(msg_container))
			{
				return INVALID_MESSAGE;
			}


			// final stage, in case of HEAP_UPDATE_MSG or PLAYER_MOVE_MSG
			if(message_type == HEAP_UPDATE_MSG)
			{
				heap_update_message* heap_msg = (heap_update_message*)msg_container;
				int i = 0;

				for(i = 0; i < NUM_HEAPS; ++i)
				{
					heap_msg->heaps[i] = ntohs(heap_msg->heaps[i]);
				}

			}
			
			else if(message_type == PLAYER_MOVE_MSG)
			{
				player_move_msg* player_move_msg = (player_move_msg*)msg_container;
				player_move_msg->amount_to_remove = ntohs(player_move_msg->amount_to_remove);
				
			}
			return SUCCESS;
		}


	}

	/* otherwise, only a part of the message is on the buffer, wait for the rest */

	return MSG_NOT_COMPLETE;

}
