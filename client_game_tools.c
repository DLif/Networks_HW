#include <stdio.h>
#include <netinet/in.h>
#include "client_game_tools.h"
#include "nim_protocol_tools.h"

/* This unit is resposible for printing various game related information to the console for the user
   such as, titles, winner id, heap represtantion, etc
*/

#define MISERE_GAME_MSG "This is a Misere game"
#define REGULAR_GAME_MSG "This is a Regular game"


/* this method prints the type of game line. game is  regular iff isMisere == 0, otherwise positive */
void print_game_type(char isMisere)
{
	printf("%s\n", isMisere ? MISERE_GAME_MSG : REGULAR_GAME_MSG);
	
}

/* method prints the game opening title */
#define GAME_TITLE "nim"
void print_title(void)
{
	printf("%s\n", GAME_TITLE);
}



#define MESSAGE_ACKED_STR "Move accepted\n"
#define MESSAGE_DECLINED_STR "Illegal move\n"
void print_message_acked( int acked )
{
	if(acked == ACK_MOVE_MSG)
	{
		printf(MESSAGE_ACKED_STR);
	}
	else
		printf(MESSAGE_DECLINED_STR);
}

#define TURN_STR "Your turn:\n"
void print_turn_message()
{
	printf(TURN_STR);
}

/* 
	print the number of items in each heap
	input: heaps - array of four shorts in NETWORK byte order
*/

void print_heaps(short* heaps)
{
	printf("Heap sizes are %d, %d, %d, %d\n", ntohs(heaps[0]), ntohs(heaps[1]), ntohs(heaps[2]), ntohs(heaps[3]));
	
}
#define DISCONNECTED_STR "Disconnected from server\n"
void print_closed_connection()
{
	printf(DISCONNECTED_STR);
}

#define CONNECTION_REFUSED "Client rejected: too many clients are already connected\n"
void print_connection_refused()
{
	printf(CONNECTION_REFUSED);
}

#define NUMBER_OF_PLAYERS  "Number of players is "
void print_num_players(char p)
{
	printf("%s%u", NUMBER_OF_PLAYERS, p);
}

#define CLIENT_ID "You are client "
void print_client_id(char id)
{
	printf("%s%u", CLIENT_ID, id);
}


#define CLIENT_SPECTATOR_MSG "You are only viewing"
#define CLIENT_PLAYER_MSG    "You are playing"

void print_client_type(char client_type)
{
	if(client_type == PLAYER)
		printf(CLIENT_PLAYER_MSG);
	else
		printf(CLIENT_SPECTATOR_MSG);
}

#define GAME_OVER_MSG "Game over!"

void print_game_over()
{
	printf(GAME_OVER_MSG);
}

#define CLIENT_WON_STR "You win!"
#define CLIENT_LOST_STR "You lose!"

void print_game_over(int win_status)
{
	if(winner == WIN)
		printf(CLIENT_WON_STR);
	else
		printf(CLIENT_LOST_STR);
}


#define PROMOTION_STR "You are now playing!\n"
void print_promotion()
{
	printf(PROMOTION_STR);
}


/*
	this method prints the openning message to the client
*/

void proccess_openning_message(openning_message* msg)
{
	print_title();
	print_game_type(msg->isMisere);
	print_num_players(msg->p);
	print_client_id(msg->client_id);
	print_client_type(msg->client_type);

}


/*

	This method checks whether the openning message recieved from the server is legal
	i.e. contains values defined in the protocol 

	returns positive value on error, otherwise returns 0
*/


int valiadate_openning_message(openning_message* msg)
{
	/* check isMisere field */
	if(msg->isMisere != MISERE && msg->isMisere != REGULAR)
	{
		return 1;
	}

	/* check client type */
	if(msg->client_type != PLAYER && msg->client_type != SPECTATOR)
	{
		return 1;
	}

	return 0;
}


