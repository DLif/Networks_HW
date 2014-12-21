#include "nim_game.h"

//this array holds the game heaps
short heaps_array[HEAPS_NUM];

//this variable tells us what kind of game this is
bool IsMisere;

/* this method initiates global parameters: heaps_array and isMisere according to the input */
void init_game(bool IsMisere_arg, short heaps_size){
	
	IsMisere = IsMisere_arg;                //init game type according to the user input
	for (int i = 0 ; i < HEAPS_NUM ; ++i) { //init heaps with the size given by the user
		heaps_array[i] = heaps_size;
	}
}

/* 
	this function makes a move in the game according to given heap index (between 0 and HEAPS_NUM-1) and how much to remove(the parameter size).
	
	NOTE: if heap_num is invalid or size is invalid, method returns false.
		  otherwise, returns true
*/
bool makeClientMove (unsigned char heap_num, unsigned short size) {
	
	if(heap_num > HEAPS_NUM - 1)
		// heap index too large
		return false;

	unsigned int_heapNum = (unsigned)heap_num;

	if ((unsigned)heaps_array[int_heapNum] < size || size == 0) 
		return false;
	
	// actually make the move
	heaps_array[int_heapNum] -= size;
	return true;
	
}

/* this function will check all heaps and determine is the game over and who's the victor;
  returns CLIENT for the client victory, NONE if the game isn't over and SERVER for server victory.
  input:
		curr_player is the last player that has made a move and changed heaps_array
		(CLIENT or SERVER)
*/

int getWinningPlayer(int current_player){
	// this variable tells us us if the game has ended, and who is the winner in such a case
	int victor = NONE;
	
	//this loop check the sizes of all the heaps - if there all empty than the game is over, else we continue;
	for (int i = 0 ; i < HEAPS_NUM ;i++) {
		if (heaps_array[i] > 0) return victor; // nobody has won yet
	}
	//if we reached this point in the code, all the heaps are empty;

	//lets calculate victory for the case  IsMisere==false
	victor = current_player;
	if (IsMisere){
		//if IsMisere == true than the loser is actualy the victor. client == 1,server == -1 => client == -server.
		//switch victor
		victor *= -1 ;
	}
	return victor;
}

int is_victory(){
	int victor = NONE;
	
	//this loop check the sizes of all the heaps - if there all empty than the game is over, else we continue;
	for (int i = 0 ; i < HEAPS_NUM ;i++) {
		if (heaps_array[i] > 0) return victor; // nobody has won yet
	}

	return SOMEONE_VICTORY;
}

/*
	this function calculates what move should the server do, and executes it.
*/
void makeServerMove(){
	//we fill the next two vars with correct data inside the loop (and update it as we advance)

	char max_heap_index = 0; //this variable holds the max index of the heap with the max size
	short max_heap_size = 0;       //this variable holds the max heap size
	

	//check all heaps, to find max size
	for (int i = 0 ; i < HEAPS_NUM ; i++){
		if (max_heap_size <= heaps_array[i]) {//if the heaps size is bigger than what we saw before (or equal).
			max_heap_size = heaps_array[i];
			//because we enter the if even if heaps size is equal to what we saw before and with each iteration the index grows, 
			//if there are several heaps with the max size, maxSize_line_index will save the one with the max index.
			max_heap_index = i;
		}
	}
	//we remove 1 piece from the biggest heap. since at this point game is not over, the move is valid.
	makeClientMove(max_heap_index, 1); 
}

/*  this function executes a whole nim game round- the user move (his move is given to the function through the parameters)
	and the computer move that follows it (only if game still continues after client move)

	returns CLIENT for the client victory, NONE if the game isn't over and SERVER for server victory.
	*isLegalMove will contain false if the user move was ilegal, else true (acked)

	for multiplay- if return SERVER curr player lost, else curr player won
*/

int makeRound(char player_heapNum, short player_size, bool *isLegalMove){
	//make the player move and save the answer to the qustion "was the move legal"
	*isLegalMove = makeClientMove((unsigned char)player_heapNum, (unsigned short)player_size);
	
	//if this is the last move, this round is over. send data
	if (is_victory() != NONE) {
		if (IsMisere)
		{
			return CLIENT;
		}
		else{
			return SERVER;
		}
	}

	// game not over
	return NONE;
}