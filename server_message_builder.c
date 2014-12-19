#include "server_message_builder.h"


//Given parametrs, this function builds the message that the server sends to the client on connection
//in case that the connection isn't allowed, all bytes except the first one will hold junk
char* init_client_message(bool connection_allowed,char client_num,char players_num,bool is_player){
	
	char * message_buf = (char *) malloc(INIT_MESSAGE_SIZE);

	//if connection isn't allowed, fill first byte accordingly, and message is ready
	if (!connection_allowed)
	{
		message_buf[0] = 0;
		return message_buf;
	}
	//init first byte with isMisere status.
	if (isMisere) 
		message_buf[1]=  IS_MISERE_TRUE; 
	else 
		message_buf[1] = IS_MISERE_FALSE;
	//init client and total players numbers
	message_buf[2] = players_num;
	message_buf[3] = client_num;
	//init is player or watcher
	if (is_player) 
		message_buf[4]=  PLAYER; 
	else 
		message_buf[4] = WATCHER;

	return message_buf;
}

//send game move message to client
char* game_message(bool is_game_over){
	short* heaps_pointer = NULL;
	char* message_buf = (char*) malloc(CLIENT_GAME_MESSAGE_SIZE);
	int i = 0;

	//set client message type
	message_buf[0] = GAME_DETAILS;
	//set is game over in last byte in the message
	if (is_game_over)
		message_buf[CLIENT_GAME_MESSAGE_SIZE-1] = GAME_OVER;
	else
		message_buf[CLIENT_GAME_MESSAGE_SIZE-1] = GAME_CONTINUES;

	//point heaps_pointer to where we sould set the heap sizes
	heaps_pointer = (short*)(message_buf+1);
	//set heap sizes
	for (i = 0; i < HEAPS_NUM; ++i)
	{
		heaps_pointer[i] = htons(heaps_array[i]);
	}

	return message_buf;
}

//sends to the client a message - did the move was valid or not
char* validity_message(bool is_valid){

	char* message_buf = (char*) malloc(MOVE_MESSAGE_SIZE);

	if (is_valid)
		message_buf[0] = VALID_TURN;
	else
		message_buf[0] = UNVALID_TURN;

	return message_buf;
}

//send "it's your turn" message
char* your_turn_message(){
	char* message_buf = (char*) malloc(MOVE_MESSAGE_SIZE);
	message_buf[0] = YOUR_TURN;
	return message_buf;
}

//sends chat message.This function copyes the message form message
//message no longer than 255. in case the message is too big return null
char* chat_message(char* message,char sender){
	int i = 0; 
	char* message_buf = NULL;
	int mess_len = (int)strlen(message);

	if (mess_len>sizeof(char))
	{
		//the message is to big
		return NULL;
	}

	message_buf = (char*) malloc(CHAT_BASE_SIZE+mess_len);
	//fill bye values
	message_buf[0] = CHAT_MESSAGE;
	message_buf[1] = sender;
	message_buf[3] = mess_len;

	//fill buffer with message 
	for (i = 0; i < (char)mess_len; ++i)
	{
		message_buf[i+4] = message[i];
	}

	return message_buf;
}

