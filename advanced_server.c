#include "advanced_server.h"

#include "nim_game.h"

buffered_socket *clients_array[];
int max_client_num = 0;
int num_of_players = 0;


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

	//init clients_array
	init_clients_array();

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
	//we will use this number to determine which player should do the next move
	int prev_client_to_move = 0;
	//game_stat will report errors or end game
	int game_stat = 0;

	while(1){
		//prepare for select
		setReadSet(&read_set,listeningSoc);
		setWriteSet(&write_set);
		fd_max = findMax(listeningSoc);

		select(fd_max+1,&read_set,&write_set,NULL,NULL);

		//check if there is a new requests to connect - and add it
		if (FD_ISSET(listeningSoc,&read_set))
		{
			game_stat = get_new_connections(listeningSoc);
			if (game_stat == NETWORK_FUNC_FAILURE)
			{
				#ifdef __DEBUG__
					printf("Error making new connection");
				#endif
				break;
			}
		}

		//get all client input and handle it
		game_stat =handle_reading_writing(&read_set,&write_set,clients_array,prev_client_to_move);
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
//return the new maximum of numbers of clients
int get_new_connections(int listeningSoc){
	//in this variable we save the client's address information
	struct sockaddr_in clientAdderInfo;
	socklen_t addressSize = sizeof(clientAdderInfo);
	//clients socket
	int toClientSocket = -1;
	//new client struct
	buffered_socket* new_client =NULL;

	//accept the connection from the client
	toClientSocket = accept(listeningSoc,(struct sockaddr*) &clientAdderInfo,&addressSize);
	if(toClientSocket < 0)
	{
		printf("Error: failed to accept connection: %s\n", strerror(errno));
		close_socket(listeningSoc);
		return NETWORK_FUNC_FAILURE;
	}

	if (max_client_num >= MAX_CLIENT_NUM)
	{
		//TODO
	}
	else{
		//add new connection to clients_array and append max_client_num
		if (num_of_players >= AWATING_CLIENTS_NUM)
		{
			new_client = create_buff_socket(toClientSocket,SPECTATOR);
			num_of_players++;
		}
		else{
			new_client = create_buff_socket(toClientSocket,REJECTED);
		}
		clients_array[max_client_num] = new_client;
		max_client_num++;
	}

	return 0;
}

//handle all input and output to clients (all the game actually)
int handle_reading_writing(fd_set* read_set,fd_set* write_set,int prev_client_to_move){
	//error indicator
	int error = 0;
	//abstract message continer
	message_container *abs_message = (message_container*)malloc(sizeof(message_container));
	//game command message
	player_move_msg *game_move = NULL;
	//get pop status
	int pop_stat = MSG_NOT_COMPLETE;
	//who should move this turn
	int curr_to_play = -1;
	//game resualt
	int round_reasult = NONE;
	bool is_leagel_move = true;

	//read all information into buffers
	error = read_to_buffs(read_set);
	if (error == NETWORK_FUNC_FAILURE)
	{
		#ifdef __DEBUG__
			printf("Errors in read_to_buffs");
		#endif
		return NETWORK_FUNC_FAILURE;
	}

	//send iformation from write buffes to ready sockes
	error = send_info(write_set);
	if (error == NETWORK_FUNC_FAILURE)
	{
		#ifdef __DEBUG__
			printf("Errors in send_info");
		#endif
		return NETWORK_FUNC_FAILURE;
	}

	//claculate who should move now
	curr_to_play = calc_next_player(prev_client_to_move);

	//pop all whole commands - and handle chat and quit
	for (int i = 0; i < max_client_num; ++i)
	{
		//if client not alive - don't try anything
		if (clients_array[i] == NULL) continue;

		pop_stat = pop_message(clients_array[i]->input_buffer,abs_message);
		if (pop_stat == MSG_NOT_COMPLETE) continue;
		else if (pop_stat == INVALID_MESSAGE)
		{
			#ifdef __DEBUG__
				printf("Errors in reading from input buffers - buffer of client %d",i);
			#endif
			return NETWORK_FUNC_FAILURE;
		}
		else { //must be pop_stat == SUCCESS
			if (abs_message->message_type == PLAYER_MOVE_MSG)
			{
				error = game_message_handle(curr_to_play,i,&abs_message,&game_move);
			}
			else if (abs_message->message_type == MSG){//
				error = chat_message_handle(i,abs_message);
			}
			else {//quit message
				error = quit_client_handle(i);
			}
		}
	}


	//handle game
	round_reasult = makeRound(game_move->heap_index,game_move->amount_to_remove,&is_leagel_move);
	free(game_move);

	//send create_heap_update_message to all
	heap_update_message* heap_mes= (heap_update_message*)malloc(sizeof(heap_update_message));
	for (int i = 0; i < max_client_num; ++i)
	{
		error = push(clients_array[j]->output_buffer,(char*)heap_mes,sizeof(heap_update_message));
	}
	free(heap_mes);

	
	if (round_reasult == NONE)
	{
		//send correct or ileagel message
		if (!is_leagel_move)
		{
			illegal_move_message *illegal_move = (illegal_move_message*)malloc(sizeof(illegal_move_message));
			//fill send buff
			create_illegal_move_message(illegal_move);
			//push message
			error = push(clients_array[curr_to_play]->output_buffer,(char*)illegal_move,sizeof(illegal_move_message));
			free(illegal_move);
		}
		else {
			ack_move_message *legal_move = (illegal_move_message*)malloc(sizeof(illegal_move_message));
			//fill send buff
			create_illegal_move_message(legal_move);
			//push message
			error = push(clients_array[curr_to_play]->output_buffer,(char*)legal_move,sizeof(illegal_move));
			free(legal_move);
		}
	}
	else {
		//send victory message to all
	}
	return 0;
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

//fill clients_array with NULLs. just to be sure
void init_clients_array(){
	int i = 0;
	for (i = 0; i < MAX_CLIENT_NUM; ++i)
	{
		clients_array[i] = NULL;
	}
}

int calc_next_player(int prev_player){
	int curr_to_play = -1;
	for (int i = prev_player; i < max_client_num; ++i)
	{
		//if client not alive - don't try anything
		if (clients_array[i] == NULL) continue;
		//if client SPECTATOR- can't move
		if (clients_array[i]->client_stat == SPECTATOR) continue;
		//else on all that-next player
		curr_to_play = i;
	}
	//circle to the begining
	for (int i = 0; i <= prev_player && curr_to_play == -1; ++i)
	{
		//if client not alive - don't try anything
		if (clients_array[i] == NULL) continue;
		//if client SPECTATOR- can't move
		if (clients_array[i]->client_stat == SPECTATOR) continue;
		//else on all that-next player
		curr_to_play = i;
	}
	return curr_to_play;
}

int chat_message_handle(int sender,message_container *abs_message){
	char* send_buff = NULL;
	int error =0;
	//this is chat message
	client_to_client_message *chat_header = (client_to_client_message*)abs_message;
	//allocate space for send_buff
	send_buff = (char*) malloc(sizeof(chat_header)+chat_header->length+1);//+1 for NULL
	//fill send_buff
	send_buff[0]=chat_header->message_type;
	send_buff[1]=chat_header->sender_id;
	send_buff[2]=chat_header->destination_id;
	send_buff[3]=chat_header->length;
	error = pop(clients_array[sender]->output_buffer,send_buff+4,chat_header->length);

	if (chat_header->destination_id == -1)
	{
		//run over all and send
		for (int i = 0; i < max_client_num; ++i)
		{
			error = push(clients_array[i]->output_buffer,send_buff,sizeof(chat_header)+chat_header->length+1);
		}
	}
	else {
		//check dest validity
		if (clients_array[chat_header->destination_id] != NULL)
		{
			error = push(clients_array[chat_header->destination_id]->output_buffer,send_buff,sizeof(chat_header)+chat_header->length+1);
		}
	}

	free(send_buff);
	return 0;
}

int game_message_handle(int curr_to_play,int curr_user,message_container** message_container_p,player_move_msg** game_move_p){
	if (curr_user == curr_to_play)
	{
		//point game_move to the new message_container
		*game_move_p = (player_move_msg*)(*message_container_p);
		//point temp message_container to new location
		*message_container_p = (message_container*)malloc(sizeof(message_container));
	}
	else{
		//send message "not your move"
		illegal_move_message *illegal_move = (illegal_move_message*)malloc(sizeof(illegal_move_message));
		//fill send buff
		create_illegal_move_message(illegal_move);
		//push message
		error = push(clients_array[curr_user]->output_buffer,(char*)illegal_move,sizeof(illegal_move_message));
		free(illegal_move);
	}
	return 0;
}

int quit_client_handle(int quiting_client){
	if (clients_array[quiting_client]->client_stat == PLAYER)
	{
		num_of_players--;
		free_buff_socket(clients_array[quiting_client]);
		//try to promote player
		for (int j = quiting_client; j < max_client_num; ++j)
		{
			if (clients_array[j] == NULL) continue;
			if (clients_array[j]->client_stat == PLAYER) continue;

			//promote one SPECTATOR
			clients_array[j]->client_stat == PLAYER;
			promotion_msg *promote_mes = (promotion_msg*)malloc(sizeof(promotion_msg));
			//fill send buff
			create_illegal_move_message(promote_mes);
			//push message
			error = push(clients_array[j]->output_buffer,(char*)promote_mes,sizeof(promotion_msg));
			free(promote_mes);
		}
	}
	else{
		free_buff_socket(clients_array[quiting_client]);
	}
}