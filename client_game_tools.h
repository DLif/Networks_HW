#ifndef _CLIENT_GAME_TOOLS
#define _CLIENT_GAME_TOOLS

/* This unit is resposible for printing various game related information to the console for the user
   such as, titles, winner id, heap represtantion, etc
*/

#include "nim_protocol_tools.h"



/* this method prints the type of game line. game is  regular iff isMisere == 0, otherwise positive */
void print_game_type(char isMisere);


/* method prints the game opening title */

void print_title(void);



void print_message_acked( int acked );

void print_turn_message();


/* 
	print the number of items in each heap
	input: heaps - array of four shorts in NETWORK byte order
*/

void print_heaps(short* heaps);

void print_closed_connection();


void print_connection_refused();


void print_num_players(char p);

void print_client_id(char id);



void print_client_type(char client_type);


void print_game_over_spectator();


void print_game_over(int win_status);


void print_promotion();



/*
	this method prints the openning message to the client
*/

void proccess_openning_message(openning_message* msg);


/*

	This method checks whether the openning message recieved from the server is legal
	i.e. contains values defined in the protocol 

	returns positive value on error, otherwise returns 0
*/


int valiadate_openning_message(openning_message* msg);



#endif