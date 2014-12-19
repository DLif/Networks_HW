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
void print_game_type(unsigned char isMisere)
{
	printf("%s\n", isMisere ? MISERE_GAME_MSG : REGULAR_GAME_MSG);
	
}

/* method prints the game opening title */
#define GAME_TITLE "nim"
void print_title(void)
{
	printf("%s\n", GAME_TITLE);
}

/* various game messages to print according to game_status byte (as defined in the protocol) */
#define SERVER_WON_STR "I win!\n"
#define CLIENT_WON_STR "You win!\n"
void print_winner(unsigned char game_status)
{
	if(hasServerWon(game_status) == 1)
	{
		printf(SERVER_WON_STR);
	}
	else
		printf(CLIENT_WON_STR);
}

#define MESSAGE_ACKED_STR "Move accepted\n"
#define MESSAGE_DECLINED_STR "Illegal move\n"
void print_message_acked(unsigned char game_status)
{
	if(lastMessageAcked(game_status))
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
