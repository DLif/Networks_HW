#include <stdbool.h>

#define HEAPS_NUM 4

/* constants define ids for client and server. NONE represents "nobody has won yet" */
#define CLIENT 1
#define NONE 0
#define SERVER -1
#define SOMEONE_VICTORY 1

//this array holds the game heaps
extern short heaps_array[HEAPS_NUM];
//this variable tells us what kind of game this is.
extern bool IsMisere;


/* this method initiates global parameters: heaps_array and isMisere according to the input */
void init_game(bool IsMisere, short heaps_size);

/* 
	this function makes a move in the game according to given heap index (between 0 and HEAPS_NUM-1) and how much to remove(the parameter size).
	heap_num and size are considered as unsigned numbers
	NOTE: if heap_num is invalid or size is invalid, method returns false[0].
		  otherwise, returns true[1]
*/
int makeClientMove (unsigned char heap_num ,unsigned short size);

/* this function will check all heaps and determine is the game over and who's the victor;
  returns CLIENT for the client victory, NONE if the game isn't over and SERVER for server victory.
  input:
		curr_player is the last player that has made a move and changed heaps_array
		(CLIENT or SERVER)
*/

int getWinningPlayer(int curr_player);

/*
	this function calculates what move should the server do, and executes it.
*/
void makeServerMove();

int is_victory();

/*  this function executes a whole nim game round- the user move (his move is given to the function through the parameters)
	and the computer move that follows it (only if game still continues after client move)

	returns CLIENT for the client victory, NONE if the game isn't over and SERVER for server victory.
	*isLegalMove will contain false if the user move was ilegal, else true (acked)
*/
int makeRound(char heap_index, short player_size, int *isLegalMove);
