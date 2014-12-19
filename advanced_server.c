#include "advanced_server.h"

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

#define NETWORK_FUNC_FAILURE -1 /* error code */
#define AWATING_CLIENTS_NUM   25 /* connection queue size */

#define __DEBUG__


int main( int argc, const char* argv[] ){
	short port = 6325;	//set the port to default
	//there will be two sockets- one listening at the port and one to comunicate with the server.
	int listeningSoc,toClientSocket;
	//in this variable we save the client's address information
	struct sockaddr_in clientAdderInfo;
	socklen_t addressSize = sizeof(clientAdderInfo);
	//this variable resives true iff an error occured during the communication with the client
	bool error = false;
	// this variable tells us if the game has ended, and who is the winner in such a case
	int victor = NONE; 

	//init the server listening socket
	listeningSoc = initServer(port);

	if (listeningSoc < 0) 
	{
		// error occured, message was already printed, quit
		return 0;
	}

#ifdef __DEBUG__
	printf("init_server_listening socket: %d, port: %d\n", listeningSoc, port);
#endif

	server_loop(listeningSoc);
}

//this function holds the server live loop
void server_loop(int listeningSoc){
	//thous are the "select variables"
	fd_set read_set;
	fd_set write_set;
	int fd_max=-1;
	//this array holds the number of all (except listening) sockets. places with 0 or -1 (decide which) are empty or dead
	int runnigSoc[AWATING_CLIENTS_NUM];
	int max_client_num = 0;//we will use the non-bouns way for now.
	//we will use this number to determine which player should do the next move
	int prev_client_to_move = 0;
	//game_stat will report errors or end game
	int game_stat = 0;

	while(1){
		//prepare for select
		setReadSet(&read_set,runnigSoc,max_client_num,listeningSoc);
		setWriteSet(&write_set,runnigSoc,max_client_num);
		fd_max = findMax(runnigSoc,max_client_num,listeningSoc);

		select(fd_max+1,&read_set,&write_set,NULL,NULL);

		//check new connection requests
		max_client_num = get_new_connections(runnigSoc,max_client_num);
		if (max_client_num == NETWORK_FUNC_FAILURE)
		{
			//print error
			break;
		}
		//get all client input and handle it
		game_stat =handle_reading_writing(&read_set,&write_set,runnigSoc,max_client_num);
		if (game_stat == NETWORK_FUNC_FAILURE)
		{
			#ifdef __DEBUG__
				printf("Error in handle_reading_writing function");
			#endif
			break;
		}
		else if (game_stat == END_GAME)
		{
			#ifdef __DEBUG__
				printf("The game has ended. Existing peacefully");
			#endif
			break;
		}
	}
}

//this function accept new connections and add them to the list(if any)
int get_new_connections(int runnigSoc[],int max_client_num){
	return NETWORK_FUNC_FAILURE;
}

//handle all input and output to clients (all the game actually)
int handle_reading_writing(fd_set* read_set,fd_set* write_set,int runnigSoc[],int max_client_num){
	return NETWORK_FUNC_FAILURE;
}

int initServer(short port){
	struct sockaddr_in adderInfo;

	//open IPv4 TCP socket with the default protocol.
	int listeningSocket= socket(PF_INET, SOCK_STREAM, 0); 
	if (listeningSocket < 0){
		printf("Error: failed to create socket: %s\n", strerror(errno));
		return NETWORK_FUNC_FAILURE;
	}
	//fill sockadder struct with correct parameters
	adderInfo.sin_family = AF_INET;
	adderInfo.sin_port = htons(port);
	adderInfo.sin_addr.s_addr= htonl( INADDR_ANY );

	// try to bind to given port
	if ( bind(listeningSocket,(struct sockaddr*) &adderInfo, sizeof(adderInfo)) < 0 ){
		printf("Error: failed to bind listening socket: %s\n", strerror(errno));
		close_socket(listeningSocket);
		return NETWORK_FUNC_FAILURE;
	}

	// listen to connections
	if ( listen(listeningSocket, AWATING_CLIENTS_NUM) < 0){
		printf("Error: listening error: %s\n", strerror(errno));
		close_socket(listeningSocket);
		return NETWORK_FUNC_FAILURE;
	}
	return listeningSocket;
}