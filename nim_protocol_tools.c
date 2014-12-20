#include "nim_protocol_tools.h"
#include "socket_IO_tools.h"

/**
	This method initializes a new openning message to the client
	use this method when the server accepts a client [CONNECTION_ACCEPTED will be used ]
	parameters:
		1. message - reference to the buffer struct to initialize
		2. isMisere: either MISERE or REGULAR [1 or 0 respectively]
		3. p - number of active players ( NOT number of active clients )
		4. client_id - id of new client, should be <= 24 at all times
		5. client_type: either SPECTATOR or PLAYER
**/

void create_openning_message(openning_message* message, char isMisere, char p, char client_id, char client_type)
{
	message->connection_accepted = CONNECTION_ACCEPTED; /* accept connection */
	message->isMisere = isMisere;
	message->p = p;
	message->client_id = client_id;
	message->client_type = client_type;

}

/** 
	this method should be used to initialize the struct when the server does not accept the connection
**/

void create_openning_message(openning_message* message)
{
	message->connection_accepted = CONNECTION_DENIED;
	/* ignore the rest */
}

/*
	method initializes a new heap update message, that the server sends to the clients
	params:
		1. message - the struct buffer, struct to initalize
		2. heaps   - array of NUM_HEAPS heaps
		3. game_over - either GAME_CONTINUES or GAME_OVER
*/

void create_heap_update_message(heap_update_message* message, short* heaps, char game_over)
{
	message->message_type = HEAP_UPDATE_MSG;
	int i;
	for(i = 0; i < NUM_HEAPS; ++i )
	{
		message->heaps[i] = heaps[i];
	}
	message->game_over = game_over;
}

/*
	method initializes a new client turn message - a message that the server sends to a specific player client
	in order to notify him that it is his turn
*/

void create_client_turn_message(client_turn_message* message)
{
	message->message_type = CLIENT_TURN_MSG;
}


/*
	these two methods initializes messages that represent:
		1. an ACK to the last move message sent by the user
		2. a negative ACK that the last move sent by the user was illegal
	repectively.
*/

void create_ack_move_message(ack_move_message* message)
{
	message->message_type = ACK_MOVE_MSG;
}

void create_illegal_move_message(illegal_move_message* message)
{
	message->message_type = ILLEGAL_MOVE_MSG;
}


/**
	method initializes a new player to player message
	params:
		sender_id - id of sender client
		destination_id - id of destination client
		length -  size of the message in bytes <= MAX_MSG_SIZE (and positive)
**/


void create_client_to_client_message(client_to_client_message* message, char sender_id, char destination_id, char length)
{
	message->message_type = MSG;
	message->sender_id = sender_id;
	message->destination_id = destination_id;
	message->length = length;

}


/*
	method initializes a promotion message, i.e. a message that a server sends to a spectator if he wishes to promote him to a player
*/


void create_promotion_message(promotion_msg* message)
{
	message->message_type = PROMOTION_MSG;
	
}


/**
	method initializes a new message that a player sends to the server if he wishes to make a move
	params:
		heap_index - index of heap that we wish to remove item from [0-3]
		amount_to_remove - amount to remove, should be a positive value <= MAX_HEAP_SIZE

**/


void create_player_move_message(player_move_msg* message, char heap_index, short amount_to_remove)
{
	message->message_type = PLAYER_MOVE_MSG;
	message->heap_index = heap_index;
	message->amount_to_remove = amount_to_remove

}


/*
	this method returns the message size in bytes according to the given id, the possible parameters are
    HEAP_UPDATE_MSG  , CLIENT_TURN_MSG , ACK_MOVE_MSG , ILLEGAL_MOVE_MSG , MSG , PROMOTION_MSG and PLAYER_MOVE_MSG

	returns a negative value if the input argument is different from the above constants.
*/



int get_message_size(int message_type)
{

	switch(message_type)
	{
	case HEAP_UPDATE_MSG:
		return sizeof(heap_update_msg);
	case CLIENT_TURN_MSG:
		return sizeof(client_turn_msg);
	case ACK_MOVE_MSG:
		return sizeof(ack_move_msg);
	case ILLEGAL_MOVE_MSG:
		return sizeof(illegal_move_msg);
	case MSG:
		return sizeof(client_to_client_msg);
	case PROMOTION_MSG:
		return sizeof(promotion_msg);
	case PLAYER_MOVE_MSG:
		return sizeof(player_move_msg);
	default:
		return -1;     /* error */

	}

}



/*

	This method reads a protocol message from server or from the client [ all messages except the openning message received by the client ]
	parameters:

		- sockfd - the socket from which to read the message
		- container - a pointer to a message_container struct (a struct that is big enough to hold any message)
		- connection_closed - will point to a positive value if the connection closed during the reading
		                      0 otherwise

	note that this method will not read sizeof(container) bytes but rather the actual size of the message
	as defined by its message type (the first byte that is read from the socket )


	return values:
		- if connection failed, failed to read from the socket, or connection was closed
		  the method will return CONNECTION_ERROR and in case that the connection was closed *connection_closed 
		  will also point to a positive value ( 0 otherwise)
	    - if the message recieved could not be identifed (invalid message type), returns INVALID_MESSAGE_HEADER
		- if the method succeed, SUCCESS is returned
*/


int read_message(int sockfd, message_container* container, int* connection_closed)
{
	
	/* read the first byte first, that defines the type of the message */
 
	if(recv_all(sockfd, (char*)container, 1, connection_closed))
	{
		/* error occured */
		return CONNECTION_ERROR;
	}

	char* message_content = ((char*)container) + 1 ;               /* skip the first byte, type of message */
	int message_size = get_message_size(container->message_type);  /* get the size of the actual struct    */

	if(message_size < 0)
	{
		/* invalid message type read from server */
		return INVALID_MESSAGE_HEADER;
	}

	if(message_size == 1)
	{
		/* we only needed to read a single byte, that defines the message */
		return SUCCESS;
	}

	/* read the rest of the message */
	if(recv_all(sockfd, message_content, message_size - 1, connection_closed))
	{
		/* error occured */
		return CONNECTION_ERROR;
	}

	/* otherwise, we have successfully read the whole message */
	return SUCCESS;


}


/*

	same as previous method
	except this method is intended for the client only to read the openning message from the server
	if the connection was refused, only a single byte of the message will be read and the method will return
	otherwise, the whole message will be read (as defined in the protocol)

	returns the same values as previous method
*/



int read_openning_message(int sockfd, openning_message* msg, int* connection_closed )
{
	
	/* read the first byte first, to whether the connection was accepted */
 
	if(recv_all(sockfd, (char*)msg, 1, connection_closed))
	{
		/* error occured */
		return CONNECTION_ERROR;
	}
	
	if(msg->connection_accepted == CONNECTION_DENIED)
	{
		/* we have read the single byte */
		return SUCCESS;
	}

	if(msg->connection_accepted != CONNECTION_ACCEPTED)
	{
		/* invalid value present */
		return INVALID_MESSAGE_HEADER;
	}

	/* else, read rest of the message */ 

	if(recv_all(sockfd, ((char*)msg) + 1, sizeof(openning_message) - 1, connection_closed))
	{
		/* error occured */
		return CONNECTION_ERROR;
	}

	return SUCCESS;

}