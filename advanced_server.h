#ifndef _ADVANCED_SERVER
#define _ADVANCED_SERVER

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
#include "server_utils.h"

#define MAX_CLIENTS  9

int main( int argc, const char* argv[] );

/*

	this method contains the main client loop that handles new connections
	handles existing connections, selects between read and write ready sockets, receives and sends information
	and handles all game related logic 
*/

void play_nim(int listeningSoc);

	

/*

	this method handles a new incoming connection 


	returns 1 on FATAL error, this method will print the proper error message
	meaning that, if the server needs to be closed, this method will return 1, otherwise 0

*/
int get_new_connections(int listeningSoc);



/*
	method initialies the listening socket on given port 
*/

int initServer(short port);

/*  
	this method checks whether the given arguments given in command line are valid
	returns true if are valid, returns false otherwise
*/ 

bool checkServerArgsValidity(int argc,const char* argv[]);


#endif