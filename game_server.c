#include "game_server.h"
#include "socket_IO_tools.h"
#include "nim_protocol_tools.h"
#include "nim_game.h"

int main( int argc, const char* argv[] ){
	short port = 6325;	//set the port to default
	short M;			//initial heaps size
	bool input_isMisere;//initial the kind of game(if the last one to finish the last heap is the winner or the loser)

	//there will be two sockets- one listening at the port and one to comunicate with the server.
	int listeningSoc,toClientSocket;

	//in this variable we save the client's address information
	struct sockaddr_in clientAdderInfo;
	socklen_t addressSize = sizeof(clientAdderInfo);

	//this variable resives true iff an error occured during the communication with the client
	bool error = false;

	// this variable tells us if the game has ended, and who is the winner in such a case
	int victor = NONE; 

	
	//check the validity of the argument given by the user.
	if (!checkServerArgsValidity(argc ,argv)){
		printf("Error: invalid arguments given to server\n");
		return 0;
	}

	// read the input from command line
	M = (short)atoi(argv[1]);
	if (atoi(argv[2]) == 0)	
		input_isMisere = false;
	else 
		input_isMisere = true;
	// check optional port argument
	if (argc == 4) 
		port = (short)atoi(argv[3]);

	//init the server listening socket
	listeningSoc = initServer(port);

	if (listeningSoc < 0) 
	{
		// error occured, message was already printed, quit
		return 0;
	}

#ifdef __DEBUG__
	printf("init_server socket: %d, port: %d\n", listeningSoc, port);
#endif


	//accept the connection from the client
	toClientSocket = accept(listeningSoc,(struct sockaddr*) &clientAdderInfo,&addressSize);

	if(toClientSocket < 0)
	{
		printf("Error: failed to accept connection: %s\n", strerror(errno));
		close_socket(listeningSoc);
		return 0;
	}

	// we can close the listening socket now
	close_socket(listeningSoc);

	//init the game according to parameters 
	init_game(input_isMisere, M);

#ifdef __DEBUG__
	printf("init_game\n");
#endif

	//play the game with the user

	//send the first message to the client (different from regular message, as defined in the protocol)
	error = sendGameDetails(toClientSocket, input_isMisere);

	//play the game while it is still not over (victor == NONE) and no error occcured (error == false)
	while(victor == NONE && error == false){

		bool islegalMove=false; //this variable tells us if the move done this round by the client is legal or not
		char player_heapNum;    //this variable holds the index of the heap the player chose this round for his move (starts with 0)
		short player_size;      //this variable holds the number of objects the player wants to take from the heap this round

		//parse message from player and save gained information.
		error = receiveAndParse(toClientSocket, &player_heapNum, &player_size);
		if (error) break;

		//make client and server move, get information about victor id (if any), and 
		//find out if client's move was valid
		victor = makeRound(player_heapNum, player_size, &islegalMove);

		#ifdef __DEBUG__
		printf("Player request: heap: %d num: %d, move legal: %d\n", player_heapNum, player_size, islegalMove);
		#endif

		//send game information to the client
		error = sendResultsOfRound(toClientSocket, islegalMove, victor);

		#ifdef __DEBUG__
		printf("Sent the round results to the client\n");
		#endif

	}
	// free resources
	close_socket(toClientSocket);

#ifdef __DEBUG__
	printf("Server exited\n");
#endif

	return 0;
}

/* 
	method allocates a socket fd for listening to incoming connections on given port number
	On success returns the listening socket fd. In case of failure returns NETWORK_FUNC_FAILURE (<0) constant 
*/

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



/*
	this method recieves the query from the client and fills the given parameters
	*player_heapNum will hold the heap index to remove from (starting from 0), *player_size will hold how many items the player
	wants to remove.
	socket is the socket with which we communicate the data to the client
	returns false if successeded, else true
*/
bool receiveAndParse(int socket, char *player_heapNum, short *player_size){
	char messageBuffer[CLIENT_QUERY_SIZE];
	
	//isConnectionClosed - tells us whether the client closed his side of the connection
	int isConnectionClosed, num_bytes = CLIENT_QUERY_SIZE;

	//fill the messageBuffer with the query 
	int error = recv_all(socket,messageBuffer,num_bytes,&isConnectionClosed);
	if (error)
	{
		if(isConnectionClosed)
		{
			printf("Error: failed to receive data, client closed connection\n");
		}
		else
		{
			printf("Error: failed to recieve data: %s\n", strerror(errno));
		}
		return true;
	}
	// fill the client move parameters according to the information stored in messageBuffer
	*player_heapNum = messageBuffer[0];
	*player_size = ntohs(((short*)(messageBuffer+1))[0]);
	return false;
}

/* 
   this method sends given message to the client via sockfd
   returns true iff error occured 
*/
bool send_server_message(int sockfd, char* buff, int num_bytes)
{
	int connection_closed;
	int error = send_all(sockfd,buff,num_bytes,&connection_closed);
	if (error) 
	{
		if (connection_closed)
		{
			printf("Error: failed to send data, client closed connection\n");
		}
		else{
			printf("Error: failed to recieve data from client: %s\n", strerror(errno));
		}
		return true;
	}
	return false;
}


/* makes and sends openning message to the client, contains: isMisere value and the heaps' sizes
input: isMisere - should be according to the game type (1 iff game is of type misere )
        socket - the socket with which we send the data to the client
returns false if successeded, else returns true on error
*/
bool sendGameDetails(int socket, bool isMisere){
	char sendBuffer[SERVER_MESSAGE_SIZE];

	//bytesSent tells the send func how many bytes to send.
	int  bytesSent = SERVER_MESSAGE_SIZE;

	//init first byte with isMisere status.
	if (isMisere) 
		sendBuffer[0]=  1; 
	else 
		sendBuffer[0] = 0;

	//this is a pointer to the place in the message buffer where we will put the heaps sizes
	short* heap_sizes = (short*)(sendBuffer+1);

	//fill the rest of the message (heap sizes)
	for(int i = 0; i < HEAPS_NUM; ++i)
	{
		heap_sizes[i] = htons(heaps_array[i]);
	}
	//Send the first message to the client

	return send_server_message(socket, sendBuffer, bytesSent);
	
}


/*
	this method sends a message to the client containing the following information:
	flags: last_time_validty (was the last move by client valid)
		   game over (victor contains the identity of the winner, if any)
		  
    sizes of each heap (four shorts)

	input: socket - connection with client
		   last_time_validity - true iff last move by client was valid (acked)
		   victor - holds SERVER, CLIENT or NONE
	returns true on error, returns false on success
*/
bool sendResultsOfRound(int socket, bool last_time_validity, int victor){

	char sendBuffer[SERVER_MESSAGE_SIZE];
	//bytesSent tells the send func how many bytes to send.

	int  bytesSent = SERVER_MESSAGE_SIZE;

	//this byte holds all the flags (last move validity, game over, who won, etc)
	unsigned char flag_container;

	//this is a pointer to the place in the message buffer where we will put the heaps sizes
	short* heap_sizes = (short*)(sendBuffer+1);

	// init container byte, contains game flags and information
	init_container(&flag_container);

	//This code chunk fills the first byte with all the nessecery information 
	switch (victor){

		//fill game over and who's victory
		case CLIENT:
#ifdef __DEBUG__
			printf("The client win\n");
#endif
			setClientWon(&flag_container);
			break;
		case SERVER:
#ifdef __DEBUG__
			printf("The server win\n");
#endif
			setServerWon(&flag_container);
			break;
		default:
			// don't turn on game over flag, nobody won
			break;
		
	}

	//if the previous move was valid, set proper bit in message (ack the message).
	if (last_time_validity)
	{
		ackClientMessage(&flag_container);
	}
	
	// start filling the buffer, first byte is the container byte
	sendBuffer[0] = (char)flag_container;

	//fill the rest of the message (heap sizes)
	for (int i = 0 ; i < HEAPS_NUM ; ++i){
		heap_sizes[i] = htons(heaps_array[i]);
	}
	
	//Send the message to the client
	return send_server_message(socket, sendBuffer, bytesSent);
}


/*  
	this method checks whether the given arguments given in command line are valid
	returns true if are valid, returns false otherwise
*/ 
#define MAX_HEAP_SIZE  1500
bool checkServerArgsValidity(int argc,const char* argv[]){

	// check if the number of arguments is valid
	if(argc > 4 || argc < 3) return false;

	errno = 0;
	// check stack size paramater
	// try to convert the given number to a long
	long stack_size = strtol(argv[1], NULL, 10);
	
	if((stack_size == LONG_MIN || stack_size == LONG_MAX) && errno != 0)
		// overflow or underflow
		return false;
	if( stack_size == 0 )
		return false;
	
	if (stack_size < 1 || stack_size > MAX_HEAP_SIZE) 
		return false;

	// check isMisere paramter
	if (argv[2][0] != '0' && argv[2][0] != '1') 
		return false;
	if (strlen(argv[2]) > 1)
		return false;
	if(argc == 4)
	{
		// check port number
		errno = 0;
		long port = strtol(argv[3], NULL, 10);
		
		if((port == LONG_MIN || port == LONG_MAX) && errno != 0)
			return false;
		if(port == 0 && errno != 0)
			return false;
		if(port < 0 || port > USHRT_MAX)
			// value cant be a port number
			return false;
	}
	return true;
}
