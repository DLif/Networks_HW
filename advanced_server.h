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
#include "server_handels_and_helper_functions.h"

#define MAX_CLIENTS  9


#define __DEBUG__



int main( int argc, const char* argv[] );

void play_nim(int listeningSoc);

int get_new_connections(int listeningSoc);

int initServer(short port);

bool checkServerArgsValidity(int argc,const char* argv[]);