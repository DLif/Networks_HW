#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "buffered_socket.h"
#include "nim_protocol_tools.h"
#include "IO_buffer.h"
#include "nim_game.h"
#include "socket_IO_tools.h"
#include "client_list.h"

#define ERROR -1 /* error code */
#define END_GAME 5

/* list of client, each client is represented by a buffered socket struct */

extern client_list clients_linked_list;

/* whose turn is it (holds the client id of the player that should move now )

   */
extern int current_turn; 

/*
	how many players are there currently 
*/

extern int current_player_num;





/*
	helper functions
*/

void update_turn(void);

int findMax(int listeningSoc);

/*
	handels
*/

int chat_message_handle(int sender, client_to_client_message *abs_message);

int game_message_handle(int curr_player , player_move_msg* message);

int quit_client_handle(int quiting_client);

int send_heaps_update(int game_over);

int send_move_ACK(int current_client, bool is_legal_move);

int read_to_buffs(fd_set* read_set);

void send_info(fd_set* write_set);

void setReadSet(fd_set* read_set,int listeningSoc);

void setWriteSet(fd_set* write_set);

int send_your_move();

int handle_ready_messages();

/*
	this function is defined in advanced server but needed here
*/

int send_final_data();