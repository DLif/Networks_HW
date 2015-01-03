#include "nim_protocol_tools.h"
#include "socket_IO_tools.h"
#include <stdio.h>
#include <netinet/in.h>

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

void create_openning_message_negative(openning_message* message)
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
		message->heaps[i] = htons(heaps[i]);
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
	message->amount_to_remove = htons(amount_to_remove);

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
		return sizeof(heap_update_message);
	case CLIENT_TURN_MSG:
		return sizeof(client_turn_message);
	case ACK_MOVE_MSG:
		return sizeof(ack_move_message);
	case ILLEGAL_MOVE_MSG:
		return sizeof(illegal_move_message);
	case MSG:
		return sizeof(client_to_client_message);
	case PROMOTION_MSG:
		return sizeof(promotion_msg);
	case PLAYER_MOVE_MSG:
		return sizeof(player_move_msg);
	default:
		return -1;     /* error */

	}

}





/*

	same as previous method
	except this method is intended for the client only to read the openning message from the server
	if the connection was refused, only a single byte of the message will be read and the method will return
	otherwise, the whole message will be read (as defined in the protocol)

	returns the same values as previous method
*/



int read_openning_message(int sockfd, openning_message* msg, int* connection_closed)
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


/*
	this method returns 0 if the given message is structually (i.e, each fields receives proper values ) valid 
	otherwise returns 1

	NOTE: 
		in case of player to player message the server still needs to check that
		the ids in the struct are valid, since only the server knows what values are valid
*/

int valiadate_message(message_container* msg)
{

	switch(msg->message_type)
	{
	case HEAP_UPDATE_MSG:
		{
			heap_update_message* temp = (heap_update_message*)msg;
			if (temp->game_over != GAME_OVER && temp->game_over != GAME_CONTINUES)
			{	
				return 1;
			}
			int i;

			
			for(i = 0; i < NUM_HEAPS; ++i)
			{
				// check if heap values are valid 
				if((unsigned short)(temp->heaps[i]) > MAX_HEAP_SIZE)
					return 1;
			}
			return 0;
		}

		
			
	case CLIENT_TURN_MSG:
		/* nothing to check */
		return 0;
	case ACK_MOVE_MSG:
		/* nothing to check */
		return 0;
	case ILLEGAL_MOVE_MSG:
		/* nothing to check */
		return 0;
	case MSG:
		{
			client_to_client_message* temp = (client_to_client_message*)msg;
			/* checks of ids should be done in the server, since it decides the valid ids */

			/* check if length is valid */
			if((unsigned char)(temp->length) > MAX_MSG_SIZE)
				return 1;
		
			return 0;
		}
	case PROMOTION_MSG:
		/* nothing to check */
		return 0;
	case PLAYER_MOVE_MSG:
		{
		
			player_move_msg* temp = (player_move_msg*)msg;
			/* check number of heaps */
			if((unsigned char)(temp->heap_index) >= NUM_HEAPS)
			{
				
				return 1;
			}
			/* check amount to remove */
			if((unsigned short)(temp->amount_to_remove) > MAX_HEAP_SIZE)
			{
				
				return 1;
			}

			return 0;

		}

	default:
		return -1;     /* error */

	}

}
