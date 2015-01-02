#include "advanced_server.h"


int main( int argc, const char* argv[] ){
	short port = 6325;	 //set the port to default
	int listeningSoc;    //listening socket on port 
	short M;			 //initial heaps size
	bool input_isMisere; //isMisere value

	//check the validity of the arguments given by the user.
	if (!checkServerArgsValidity(argc ,argv)){
		printf("Error: invalid arguments given to server\n");
		return 0;
	}

	// read the input from command line
	max_players_allowed = atoi(argv[1]);
	M = (short)atoi(argv[2]);
	if (atoi(argv[3]) == 0)	
		input_isMisere = false;
	else 
		input_isMisere = true;

	// check optional port argument
	if (argc == 5) 
		port = (short)atoi(argv[4]);

	//init game
	init_game(input_isMisere, M);

	//init the server listening socket
	listeningSoc = initServer(port);

	if (listeningSoc < 0) 
	{
		/* error message already printed in initServer */
		return 0;
	}

	//init linked list
	init_client_list(&clients_linked_list);

	play_nim(listeningSoc);

	// free resources

	close_socket(listeningSoc);


	/* free all the client structs and close all the file descriptors */
	free_list(&clients_linked_list, true);

}


/*

	this method contains the main client loop that handles new connections
	handles existing connections, selects between read and write ready sockets, receives and sends information
	and handles all game related logic 
*/

void play_nim(int listeningSoc){
	fd_set read_set;
	fd_set write_set;
	int fd_max= -1;
	
	int error = 0;

	while(1){

		//prepare for select
		setReadSet(&read_set,listeningSoc);
		setWriteSet(&write_set);
		fd_max = findMax(listeningSoc);

		error = select(fd_max+1,&read_set,&write_set,NULL,NULL);
		if (error < 0)
		{
			printf("Error: select method failed %s\n", strerror(errno));
			return;
		}

		//check if there is a new requests to connect - and add it
		if (FD_ISSET(listeningSoc,&read_set))
		{
			
			if( get_new_connections(listeningSoc))
			{
				/* error is printed in the method itself */
				/* this is a fatal error, memory must be freed and server closed */

				return;
			}
		}

		// read information from buffers
		if(read_to_buffs(&read_set) == ERROR)
		{
			/* fatal error, full buffer or network error */

			return;
		}

		// see if there are ready messages to be handled

		if((error = handle_ready_messages()) == ERROR)
		{
			/* fatal error, could not handle messages, some invalid message was found, or network error */
			return;
		}

		if(error == END_GAME)
		{
			// game successfully ended
			return;
		}


		//send iformation from write buffes to ready sockes
		// method returns void, drops problematic connections
		send_info(&write_set); 
	

	}
	

}

/*

	this method handles a new incoming connection 


	returns 1 on FATAL error, this method will print the proper error message
	meaning that, if the server needs to be closed, this method will return 1, otherwise 0

*/
int get_new_connections(int listeningSoc){


	//in this variable we save the client's address information
	struct sockaddr_in clientAdderInfo;
	socklen_t addressSize = sizeof(clientAdderInfo);

	//clients socket
	int toClientSocket = -1;

	//new client struct
	buffered_socket* new_client =NULL;

	//first message that will be sent to the client 
	openning_message first_msg;

	//accept the connection from the client
	toClientSocket = accept(listeningSoc,(struct sockaddr*) &clientAdderInfo,&addressSize);
	if(toClientSocket < 0)
	{
		printf("Error: failed to accept connection: %s\n", strerror(errno));
		return 1;
	}

	/* get the next id to be allocated (minimum that is not taken ) */
	int new_client_id = get_minimum_free_client_id(&clients_linked_list);

	if (clients_linked_list.size == MAX_CLIENTS)
	{

		/* we've already received MAX_CLIENTS connections, must decline this connection */
		int is_connection_closed = 0;

		//assume working and write ready
		openning_message invalid_message;
		create_openning_message_negative(&invalid_message);
		
		if(send_partially(toClientSocket,(char*)(&invalid_message),1, &is_connection_closed) < 0)
		{
			printf("Error: failed to send a declining message to client, closing his connection and continuing: %s\n", strerror(errno));
		}
		close_socket(toClientSocket);
		return 0;
		
		
	}
	else if(current_player_num >= max_players_allowed){

		/* we have room for more clients, but we've reached the maximum player limit */
		/* this client will be a spectator */
		
		
		/* allocate a new buffer abstraction */
		new_client = create_buff_socket(toClientSocket, SPECTATOR, new_client_id);
		
		if(new_client == NULL)
		{
			// malloc failed
			close_socket(toClientSocket);
			return 1;
		}


		/* add the spectator to the list (append to the end) */
		add_client(&clients_linked_list, new_client);

	}
	else{
		

		/* we have room for more players */
		new_client = create_buff_socket(toClientSocket, PLAYER, new_client_id);

		if(new_client == NULL)
		{
			// malloc failed
			close_socket(toClientSocket);
			return 1;
		}
		
		++current_player_num;

		/* add the player as the last player */
		add_player(&clients_linked_list, new_client, current_turn);
		
		
	}
	

	// create openning message (accepting message with the relevant info )
	create_openning_message(&first_msg, IsMisere, max_players_allowed, new_client_id, new_client->client_stat);


	if(push(new_client->output_buffer,(char*)(&first_msg), sizeof(openning_message)))
	{
		printf("Error: unable to push first message to client %d\n", new_client_id);
		return 1;

	}


	// create heap update message

	heap_update_message heap_msg;
	// GAME_CONTINUES since we wouldnt be here if game was over
	create_heap_update_message(&heap_msg, heaps_array, GAME_CONTINUES);

	if ( push(new_client->output_buffer,(char*)(&heap_msg), sizeof(heap_update_message)) )	
	{
		printf("Error: could not push update message onto client %d output buffer (buffer full)\n", new_client->client_id);
		return ERROR;
	}


	

	if(new_client->client_stat == PLAYER && current_player_num == 1)
	{
		// finally a player has connected
		current_turn = new_client_id;
		// notify the player that it is his turn
		if(send_your_move()== ERROR)
			return ERROR;
		
	}

	return 0; // OK
}

int initServer(short port){
	struct sockaddr_in adderInfo;

	//open IPv4 TCP socket with the default protocol.
	int listeningSocket= socket(PF_INET, SOCK_STREAM, 0); 
	if (listeningSocket < 0){
		printf("Error: failed to create socket: %s\n", strerror(errno));
		return ERROR;
	}
	//fill sockadder struct with correct parameters
	adderInfo.sin_family = AF_INET;
	adderInfo.sin_port = htons(port);
	adderInfo.sin_addr.s_addr= htonl( INADDR_ANY );

	// try to bind to given port
	if ( bind(listeningSocket,(struct sockaddr*) &adderInfo, sizeof(adderInfo)) < 0 ){
		printf("Error: failed to bind listening socket: %s\n", strerror(errno));
		close(listeningSocket);
		return ERROR;
	}

	// listen to connections
	if ( listen(listeningSocket, MAX_CLIENTS+1) < 0){
		printf("Error: listening error: %s\n", strerror(errno));
		close(listeningSocket);
		return ERROR;
	}
	return listeningSocket;
}





/*  
	this method checks whether the given arguments given in command line are valid
	returns true if are valid, returns false otherwise
*/ 

bool checkServerArgsValidity(int argc,const char* argv[]){

	// check if the number of arguments is valid
	if(argc > 5 || argc < 4) return false;

	errno = 0;
	//check p parameter

	long p = strtol(argv[1], NULL, 10);
	if((p == LONG_MIN || p == LONG_MAX) && errno != 0)
		// overflow or underflow
		return false;
	if( p == 0 )
		return false;
	
	if (p < 2 || p > MAX_CLIENTS) 
		return false;

	// check stack size paramater
	// try to convert the given number to a long
	long stack_size = strtol(argv[2], NULL, 10);
	
	if((stack_size == LONG_MIN || stack_size == LONG_MAX) && errno != 0)
		// overflow or underflow
		return false;
	if( stack_size == 0 )
		return false;
	
	if (stack_size < 1 || stack_size > MAX_HEAP_SIZE) 
		return false;

	// check isMisere paramter
	if (argv[3][0] != '0' && argv[3][0] != '1') 
		return false;
	if (strlen(argv[3]) > 1)
		return false;
	if(argc == 5)
	{
		// check port number
		errno = 0;
		long port = strtol(argv[4], NULL, 10);
		
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