#include "advanced_server.h"

/* list of client, each client is represented by a buffered socket struct */

client_list clients_linked_list;

/* max PLAYERS number as received from the input args (p value as defined in the pdf) */

int max_players_allowed = 3;


/* whose turn is it (holds the client id of the player that should move now )

   */
int current_turn =  0; 

/*
	how many players are there currently 
*/

int current_player_num = 0;
 


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
	max_players_allowed= atoi(argv[1]);
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
			printf("new client\n");
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
		printf("new SPECTATOR number %d\n", new_client_id);
		
		/* allocate a new buffer abstraction */
		new_client = create_buff_socket(toClientSocket, SPECTATOR, new_client_id);
		
		if(new_client == NULL)
		{
			// malloc failed
			close_socket(toClientSocket);
			return 1;
		}

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
		printf("new PLAYER number %d\n", new_client_id);
		++current_player_num;
		
		
	}
	/* add the client to the list */
	add_client(&clients_linked_list, new_client);

	// create openning message (accepting message with the relevant info )
	create_openning_message(&first_msg, IsMisere, max_players_allowed, new_client_id, new_client->client_stat);

	printf("should push %d bytes\n", sizeof(openning_message));

	if(push(new_client->output_buffer,(char*)(&first_msg), sizeof(openning_message)))
	{
		printf("Error: unable to push first message to client %d\n", new_client_id);
		return 1;

	}
	
	printf("head: %d, tail %d, size %d \n", new_client->output_buffer->head, new_client->output_buffer->tail, new_client->output_buffer->size);

	if(new_client->client_stat == PLAYER && current_player_num == 1)
	{
		// finally a player has connected
		current_turn = new_client_id;
		// notify the player that it is his turn
		if(send_your_move()== ERROR)
			return ERROR;
		printf("head: %d, tail %d, size %d \n", new_client->output_buffer->head, new_client->output_buffer->tail, new_client->output_buffer->size);
	}

	return 0; // OK
}








/*

	this method pops whole messages, and handles
	valid messages may be of types MSG (peer to peer message)
	or PLAYER_MOVE_MSG ( some player wishes to make a move )

	if some error occured, a proper message will be printed, and ERROR returned
	if game successfully ended, returns END_GAME
	o/w 0 is returned

*/

int handle_ready_messages()
{

	buffered_socket* client;
	message_container abs_message;
	int err, pop_stat;

	//pop all whole commands - and handle chat and quit
	for(client = clients_linked_list.first; client != NULL; client = client->next_client)
	{
		pop_stat = pop_message(client->input_buffer, &abs_message);

		if (pop_stat == MSG_NOT_COMPLETE) { continue; }

		else if (pop_stat == INVALID_MESSAGE)
		{
			//undefined message

			printf("Error: invalid message received from client id %d\n", client->client_id );
			return ERROR;
		}
		else { 

			//must be pop_stat == SUCCESS
			if (abs_message.message_type == PLAYER_MOVE_MSG)
			{
				if((err = game_message_handle( client->client_id,(player_move_msg*)(&abs_message))) == ERROR)
				{
					// error
					return ERROR;
				}
				else if(err == END_GAME)
					return END_GAME;
			}
			else if (abs_message.message_type == MSG){

				if(chat_message_handle(client->client_id, (client_to_client_message*)(&abs_message)))
				{
					// error
					return ERROR;
				}
			}
			else { 

				//error in message type, message intented to be sent to clients
				printf("Error: invalid message received from client id %d\n", client->client_id);
				return ERROR;
			}
		}
	}

	// OK
	return 0;

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





/* find the next player to play */
void update_turn(){

	buffered_socket* prev_client = get_buffered_socket_by_id(current_turn);

	// update turn, find next player

	current_turn = get_next_player_id(&clients_linked_list, prev_client->next_client);
}


/*

	method finds a buffered socket from the list by given req_id (should be non negative)
	if no such client is found, NULL is returned 
*/

buffered_socket* get_buffered_socket_by_id(int req_id){

	buffered_socket *running = NULL;
	for(running = clients_linked_list.first; running != NULL; running = running->next_client){
		if (running->client_id == req_id)
		{
			return running;
		}
	}


	return NULL;
}




/*
	this method handles a client to client message request
	method also handles errors (is the message actually valid, output buffers overflow)

*/

int chat_message_handle(int sender_id, client_to_client_message *message){


	buffered_socket * running = NULL;
	

	// allocate temp buffer to read the message to 
	char temp_buff[MAX_MSG_SIZE];

	// check if message is legal

	if(sender_id != (unsigned char)(message->sender_id))
	{
		// the client entered a wrong sender id
		// invalid message
		printf("Error: client %d tried to send a messsage but entered a wrong sender id %u\n", sender_id, message->sender_id);
		return ERROR;
	}


	// error cannot happen here, we know we can safely pop the message (verified by pop_message)
	pop(get_buffered_socket_by_id(sender_id)->input_buffer, temp_buff, (unsigned char)(message->length));
	
	if (message->destination_id == -1)
	{
		// broadcast
		for(running = clients_linked_list.first; running != NULL; running = running->next_client)
		{
			// modify the header before sending it
			message->destination_id = running->client_id;

			// push the header itself
			if(push(running->output_buffer, (char*)message, sizeof( client_to_client_message )))
			{
				printf("Error: could not push message onto client %u buffer (BUFFER FULL!)\n", running->client_id);
				return ERROR;
			}

			// push the message itself
			if( push(running->output_buffer, temp_buff, (unsigned char)(message->length)) )
			{
				printf("Error: could not push message onto client %u buffer (BUFFER FULL!)\n", running->client_id);
				return ERROR;
			}
		}
	}
	else {

		// check dest validity and send to specific client

		buffered_socket* dest_buff_socket = get_buffered_socket_by_id( (unsigned char)(message->destination_id));
		if (dest_buff_socket != NULL)
		{
			// send the message

			// push the header itself
			if(push(dest_buff_socket->output_buffer, (char*)message, sizeof( client_to_client_message )))
			{
				printf("Error: could not push message onto client %u buffer (BUFFER FULL!)\n", dest_buff_socket->client_id);
				return ERROR;
			}

			// push the message itself
			if( push(dest_buff_socket->output_buffer, temp_buff, (unsigned char)(message->length)) )
			{
				printf("Error: could not push message onto client %u buffer (BUFFER FULL!)\n", dest_buff_socket->client_id);
				return ERROR;
			}

			temp_buff[3] = 0;
			printf("pushed message %s\n", temp_buff);
		}
		// otherwise, ignore it (note that we really poped it )
	}

	// OK
	return 0;
}








/* 
	handles a game move message from client
	returns ERROR if some error occured
	returns END_GAME if game successfully ended 
*/


int game_message_handle(int client_id, player_move_msg* message){

	int error = 0;

	if (current_turn == client_id)
	{
		// handle move
		printf("client %d made turn\n", client_id);
		int is_legal_move;

		// actually make the move
		int round_result = makeRound(message->heap_index, message->amount_to_remove, &is_legal_move);
		
		// send the ack to the client
		error = send_move_ACK(client_id, is_legal_move);
		if(error == ERROR)
			return ERROR;

		// send update message to all the clients

		if (round_result == NONE)
		{
			// game not over
			
			if( send_heaps_update(GAME_CONTINUES))
			{
				return ERROR;
			}

			// set next turn (next item in list )
			update_turn();

			// notify new player that it is his turn

			if(send_your_move())
			{
				return ERROR;
			}
			
		
		}
		else {
			
			// game over

			if( send_heaps_update(GAME_OVER))
			{
				return ERROR;
			}


			if(send_final_data())
			{
				return ERROR;
			}

			return END_GAME;
		}


	}
	else{

		//send message "not your move"
		illegal_move_message illegal_move;

		//fill send buff
		create_illegal_move_message(&illegal_move);

		//push message
		error = push(get_buffered_socket_by_id(client_id)->output_buffer,(char*)(&illegal_move),sizeof(illegal_move_message));
		if (error == OVERFLOW_ERROR)
		{
			printf("Error: could not push message onto client %d output buffer (buffer full)\n", client_id);
			return ERROR;
		}
	}
	// OK
	return 0;
}

int quit_client_handle(int quiting_client){

	int error =0;

	printf("quiting client %d\n", quiting_client);

	if (get_buffered_socket_by_id(quiting_client)->client_stat == PLAYER)
	{
		current_player_num--;


		// save pointer of next client in the list (to determine next player turn if needed)
		buffered_socket* next_player = get_buffered_socket_by_id(quiting_client)->next_client;
		if(next_player == NULL) next_player = clients_linked_list.first;

		int update_turn = false;

		if(quiting_client == current_turn)
		{
			update_turn = true;
		}

		// delete
		delete_by_client_id(&clients_linked_list,quiting_client);


		//try to promote player
		int new_player_id = get_min_spectator_id(&clients_linked_list);

		

		if(new_player_id == MAX_CLIENTS + 1)
		{	
			// no one to promote
			
			if(current_player_num > 0 && update_turn)
			{
				current_turn = get_next_player_id(&clients_linked_list, next_player);
				/* notify the new player */
				if(send_your_move() == ERROR)
					return ERROR;
			}
			if(current_player_num == 0)
			{
				printf("reached here\n");
				printf("%d %d %d\n", clients_linked_list.size, (int)clients_linked_list.first, (int)clients_linked_list.last);
				current_turn = 0; /* pause the game */
			}

			return 0;
		}

		// else, gotta promote new_player_id

		promotion_msg promote_mes;
		create_promotion_message(&promote_mes);

		buffered_socket* client_sock = get_buffered_socket_by_id(new_player_id);

		//push message
		error = push(client_sock->output_buffer,(char*)(&promote_mes),sizeof(promotion_msg));
		if (error == OVERFLOW_ERROR)
		{
			printf("Error pushing promote_messsage to client %d\n", new_player_id);
			return ERROR;
		}

		// successfully promoted
		++ current_player_num;


		if(update_turn)
		{
			/* there is atleast one player at this point */
			current_turn = get_next_player_id(&clients_linked_list, next_player);
			if(send_your_move() == ERROR)
				/* notify the new player */
				return ERROR;
		}
		

	}
	else
	{
		delete_by_client_id(&clients_linked_list,quiting_client);
	}
	
	return 0;
}



/*

	method "sends" a heap update message to all the clients
	(pushes on output buffers)
	on error, returns ERROR o/w returns 0
*/

int send_heaps_update(int game_over){

	buffered_socket* running = NULL; /* iterator */
	int error ;

	printf("send heaps to all\n");
	heap_update_message heap_msg;
	create_heap_update_message(&heap_msg, heaps_array, (char)game_over);

	for(running = clients_linked_list.first; running != NULL; running=running->next_client)
	{
		error = push(running->output_buffer,(char*)(&heap_msg), sizeof(heap_update_message));
		if (error == OVERFLOW_ERROR)
		{
			printf("Error: could not push update message onto client %d output buffer (buffer full)\n", running->client_id);
			return ERROR;
		}

		if(game_over == GAME_OVER)
		{
			char status;
			if(running->client_id == current_turn)
			{

				if(IsMisere == false)
				{
					// winner is the last player who moved
					status = WIN;
				}
				else
				{
					// loser is the last player who moved
					status = LOSE;
				}
				
			}
			else if(running->client_stat == PLAYER)
			{
				if(IsMisere == false)
					status = LOSE;
				else
					status = WIN;
			}

			if(running->client_stat == PLAYER)
			{

				// push the message
				error = push(running->output_buffer, &status, 1);
				if (error == OVERFLOW_ERROR)
				{
						printf("Error: could not push update message onto client %d output buffer (buffer full)\n", running->client_id);
						return ERROR;
				}
			}

		}

	}



	// OK
	return 0;
}




/*

	method handles sending ACK or NACK to current player, according to is_legal_move
	
	returns ERROR on error, or 0 if successfull

*/
	
int  send_move_ACK(int current_client, bool is_legal_move){

	int error;

	if (is_legal_move == false)
	{
		illegal_move_message illegal_move;
		//fill send buff
		create_illegal_move_message(&illegal_move);
		//push message
		error = push(get_buffered_socket_by_id(current_client)->output_buffer,(char*)(&illegal_move),sizeof(illegal_move_message));
		if (error == OVERFLOW_ERROR)
		{
			printf("Error: can't send negative ACK to client %d (output buffer full)\n", current_client);
			return ERROR;
		}
	}
	else {
		ack_move_message legal_move;

		//fill send buff
		create_ack_move_message(&legal_move);

		//push message
		error = push(get_buffered_socket_by_id(current_client)->output_buffer,(char*)(&legal_move),sizeof(ack_move_message));
		if (error == OVERFLOW_ERROR)
		{
			printf("Error: can't send ACK to client %d (output buffer full)\n", current_client);
			return ERROR;
		}
	}
	return 0;
}

/*

	iterate over the clients in the read_set, and receive as much as you can from each client socket
	(fill input buffers)
	on any error, a proper message is printed and ERROR is returned
	on success, 0 is returned

	NOTE: this method also handles closed connections
*/
int read_to_buffs(fd_set* read_set){

	buffered_socket *p, * next;

	int cur_socket ;
	/* temp buffer to store the read data, will be used as a middle man*/
	char recv_buff[MAX_IO_BUFFER_SIZE];
	int bytes_recv =0;

	p = clients_linked_list.first; /* list iterator */

	while(p != NULL)
	{
		cur_socket = p->sockfd;

		if (FD_ISSET(cur_socket,read_set)){

			printf("reading from socket\n");
			/* check if socket is read ready */
			bytes_recv = recv(cur_socket, recv_buff, MAX_IO_BUFFER_SIZE, 0);
			if (bytes_recv < 0)
			{
				printf("Error: could not read from client id %d, error: %s \n", p->client_id , strerror(errno));
				return ERROR;
			}
			else if (bytes_recv == 0) 
			//in this case - connection closed by user 
			{
				printf("reading from socket, recv 0 \n");
				// save next client
				next = p->next_client;
				printf("next: %s\n", next == NULL ? "null" : "something");
				quit_client_handle(p->client_id);
				p = next;
				continue;
			}
			// try to push recieved data onto the input buffer of client
			if( push(p->input_buffer, recv_buff, bytes_recv))
			{
				printf("Error pushing to input buffer of client number %d (input buffer is full !)\n", p->client_id);
				return ERROR;
			}
		}
		//advance loop
		p= p->next_client;
	}

	return 0;
}


/*

	method sends the output buffers (does not use sendall, sends how much is premitted by send 
	to the client sockets
	if a connection is closed suddenly it is simply dropped and an error message will be printed
*/

void send_info(fd_set* write_set){

	buffered_socket *running_next = NULL;
	buffered_socket *running      = NULL;
	
	int cur_socket;
	int bytes_read =0;


	for(running = clients_linked_list.first; running != NULL;)
	{
		cur_socket = running->sockfd;

		if (FD_ISSET(cur_socket, write_set) && running->output_buffer->size > 0){
			int is_connection_closed = 0 ;

			//try to send all the buffer
			bytes_read = send_partially_from_buffer(running->output_buffer, cur_socket , running->output_buffer->size, &is_connection_closed);
			if (bytes_read < 0)
			{

				running_next = running->next_client;
				quit_client_handle(running->client_id);
				running = running_next;

				if(is_connection_closed)
				{
					printf("Error: client %d closed connection, client will be dropped\n", running->client_id );
				}
				else
					printf("Error sending to socket of client, client will be dropped %d\n", running->client_id  );
				continue;

			}
		
			
			printf("to client %d sent %d bytes\n", running->client_id, bytes_read);
			
		
			//pop all the information we were able to send
			pop_no_return(running->output_buffer, bytes_read);
			
		}

		//advance loop
		running = running->next_client;
	}

}

void setReadSet(fd_set* read_set, int listeningSoc){

	buffered_socket *running = NULL;

	FD_ZERO(read_set);
	if (listeningSoc != -1)
	{
		FD_SET(listeningSoc, read_set);
	}
	for(running=clients_linked_list.first; running != NULL; running=running->next_client)
	{
		FD_SET(running->sockfd,read_set);
	}
}

void setWriteSet(fd_set* write_set){
	buffered_socket *running = NULL;

	FD_ZERO(write_set);
	for(running=clients_linked_list.first;running != NULL;running=running->next_client)
	{
		FD_SET(running->sockfd, write_set);
	}
}



//this function will find the max socket number. used for select
int findMax(int listeningSoc){
	buffered_socket *running = NULL;

	int max = listeningSoc;
	for(running=clients_linked_list.first;running != NULL;running=running->next_client)
	{
		if (max < running->sockfd)
		{
			max = running->sockfd;
		}
	}
	return max;
}


/*

	after the game has ended, this method
	tries to send the information (using select) untill all players either closed connection or all data was sent

	returns ERROR on error, 0 otherwise 
*/


int send_final_data(){


	fd_set read_set;
	fd_set write_set;

	int fd_max;
	int error =0;

	buffered_socket *running = NULL;
	buffered_socket *running_next = NULL;

	while(1){

		bool finished_sending = true;

		if(clients_linked_list.size == 0)
		{
			// client list is empty
			return 0;
		}

		setReadSet(&read_set, -1); // -1 stands for: do not add listening socket
		setWriteSet(&write_set);
		fd_max = findMax(-1);     //we dont try to read listening socket now

		error = select(fd_max+1,&read_set,&write_set,NULL,NULL);
		if (error <0 )
		{
			printf("Error: select failed %s\n", strerror(errno));
			return ERROR;
		}

		printf(" .. emptying buffers\n");

		//check if all buffers are empty and check who quit

		for(running = clients_linked_list.first ; running != NULL; )
		{
			if (FD_ISSET(running->sockfd,&read_set))
			{
				char temp;
				if(recv(running->sockfd, &temp, 1, 0) <= 0)
				{
					// connection closed

					running_next = running->next_client;

					delete_by_client_id(&clients_linked_list, running->client_id);

					running = running_next; //this is because now running points to junk
					continue;
				}
				
			}
			if (running->output_buffer->size > 0)
			{
				finished_sending = false;
			}
	

			running = running->next_client; 
		
		}


		if (!finished_sending)
		{
			// send to all the write ready
			send_info(&write_set);
	
		}
		else{
			// finished sending all sockets, may exit
			return 0;
		}
	}
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

/*
	method notifies next_player that it is his move
*/

int send_your_move(){

	int error =0;
	printf("send your_turn_move to client %d\n", current_turn);

	client_turn_message your_turn_move;
	//fill send buff
	create_client_turn_message(&your_turn_move);
	//push message
	error = push(get_buffered_socket_by_id(current_turn)->output_buffer,(char*)(&your_turn_move),sizeof(client_turn_message));
	
	if (error == OVERFLOW_ERROR)
	{
		printf("Error: can't push client_turn_message to %d\n", current_turn);
		return ERROR;
	}
	return 0;
}
