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
#define MAX_CLIENTS   9


#define __DEBUG__



int main( int argc, const char* argv[] );

void play_nim(int listeningSoc);

int get_new_connections(int listeningSoc);



int initServer(short port);


void update_turn(void);

buffered_socket* get_buffered_socket_by_id(int req_id);

int chat_message_handle(int sender, client_to_client_message *abs_message);

int game_message_handle(int curr_player , player_move_msg* message);

int quit_client_handle(int quiting_client);

int send_heaps_update(int game_over);

int send_move_ACK(int current_client, bool is_legal_move);

int read_to_buffs(fd_set* read_set);

void send_info(fd_set* write_set);

void setReadSet(fd_set* read_set,int listeningSoc);

void setWriteSet(fd_set* write_set);

int findMax(int listeningSoc);

int send_final_data();

bool checkServerArgsValidity(int argc,const char* argv[]);

int send_your_move();

int handle_ready_messages();