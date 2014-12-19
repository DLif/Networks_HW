#include <stdbool.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

//init message definitions
#define INIT_MESSAGE_SIZE 5
#define IS_MISERE_FALSE 0
#define IS_MISERE_TRUE 1
#define WATCHER 0
#define PLAYER 1

//Message type byte definitions
#define CHAT_MESSAGE 0
#define GAME_DETAILS 1
#define YOUR_TURN 2
#define VALID_TURN 3
#define UNVALID_TURN 4

#define MOVE_MESSAGE_SIZE 1

//game message definitions
#define CLIENT_GAME_MESSAGE_SIZE (2+sizeof(char)*4)//in bytes!
#define GAME_OVER 1
#define GAME_CONTINUES 0
#define HEAPS_NUM 4

//chat message definitions
#define CHAT_BASE_SIZE (2+sizeof(char))

//Given parametrs, this function builds the message that the server sends to the client on connection
//in case that the connection isn't allowed, all bytes except the first one will hold junk
char* init_client_message(bool connection_allowed,char client_num,char players_num,bool is_player);

//send game move message to client
char* game_message(bool is_game_over);

//sends to the client a message - did the move was valid or not
char* validity_message(bool is_valid);

//send "it's your turn" message
char* your_turn_message();

//sends chat message.This function copyes the message form message
//message no longer than 255. in case the message is too big return null
char* chat_message(char* message,char sender);