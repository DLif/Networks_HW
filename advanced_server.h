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

#define NETWORK_FUNC_FAILURE -1 /* error code */
#define END_GAME 1
#define MAX_CLIENTS   9


#define __DEBUG__



int main( int argc, const char* argv[] );

int server_loop(int listeningSoc);

int get_new_connections(int listeningSoc);

int handle_reading_writing(fd_set* read_set,fd_set* write_set,int *curr_to_play,bool *first_move);

int initServer(short port);

void init_clients_array();

int calc_next_player(int prev_player);

buffered_socket* get_buffered_socket_by_id(int req_id);

int chat_message_handle(int sender,message_container *abs_message);

int game_message_handle(int curr_to_play,int curr_user,message_container** message_container_p,player_move_msg** game_move_p);

int quit_client_handle(int quiting_client);

int send_heap_mss_to_all();

int  send_move_leagelness(int curr_to_play,bool is_leagel_move);

int read_to_buffs(fd_set* read_set);

int send_info(fd_set* write_set);

void setReadSet(fd_set* read_set,int listeningSoc);

void setWriteSet(fd_set* write_set);

int findMax(int listeningSoc);

int send_msg_affter_endGame();

void calc_min_by_new_free(int new_free);

void calc_new_min_by_occupy();

bool checkServerArgsValidity(int argc,const char* argv[]);

int send_your_move(int next_player);