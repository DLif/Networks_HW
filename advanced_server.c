#include "advanced_server.h"

buffered_socket *clients_array[MAX_CLIENT_NUM];
int max_client_num = 0;
int num_of_players = 0;


int main( int argc, const char* argv[] ){
	short port = 6325;	//set the port to default
	int listeningSoc; //listening socket on port

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

	//free all clients_array
	for (int i = 0; i < max_client_num; ++i)
	{
		//if client not alive - don't try anything
		if (clients_array[i] == NULL) continue;

		free_buff_socket(clients_array[i]);
	}
}

//this function holds the server live loop
int server_loop(int listeningSoc){
	//thous are the "select variables"
	fd_set read_set;
	fd_set write_set;
	int fd_max=-1;
	//we will use this number to determine which player should do the next move
	int prev_client_to_move = 0;
	//game_stat will report errors or end game
	int game_stat = 1;
	int error =0;

	while(1){
		//prepare for select
		setReadSet(&read_set,listeningSoc);
		setWriteSet(&write_set);
		fd_max = findMax(listeningSoc);

		error = select(fd_max+1,&read_set,&write_set,NULL,NULL);
		if (error == NETWORK_FUNC_FAILURE)
		{
			printf("error in select ");
			return NETWORK_FUNC_FAILURE;
		}

		if (game_stat == END_GAME)
		{
			bool finished_sending = true;
			//check if all buffers are empty
			for (int i = 0; i < max_client_num; ++i)
			{
				//if client not alive - don't try anything
				if (clients_array[i] == NULL) continue;
				if (clients_array[i]->output_buffer->size > 0)
				{
					finished_sending = false;
				}
			}

			if (!finished_sending)
			{
				error = send_info(&write_set); 
				if (error == NETWORK_FUNC_FAILURE)
				{
					printf("Errors in sending after game end");
					return NETWORK_FUNC_FAILURE;
				}
			}
			else{
				return 0;
			}
			continue;
		}

		//check if there is a new requests to connect - and add it
		if (FD_ISSET(listeningSoc,&read_set))
		{
			game_stat = get_new_connections(listeningSoc);
			if (game_stat == NETWORK_FUNC_FAILURE)
			{
				printf("Error making new connection");
				return NETWORK_FUNC_FAILURE;
			}
		}

		//get all client input and handle it
		game_stat =handle_reading_writing(&read_set,&write_set,&prev_client_to_move);
		if (game_stat == NETWORK_FUNC_FAILURE)
		{
			printf("Error in handle_reading_writing function");
			return NETWORK_FUNC_FAILURE;
		}
		else if (game_stat == END_GAME)
		{
			printf("The game has ended. Existing peacefully");
		}
	}
	//should never get here
	return NETWORK_FUNC_FAILURE;
}

//this function accept new connections and add them to the list(if any)
//return the new maximum of numbers of clients
int get_new_connections(int listeningSoc){
	int error = 0;
	//in this variable we save the client's address information
	struct sockaddr_in clientAdderInfo;
	socklen_t addressSize = sizeof(clientAdderInfo);
	//clients socket
	int toClientSocket = -1;
	//new client struct
	buffered_socket* new_client =NULL;
	//first message struct
	openning_message first_msg;

	//accept the connection from the client
	toClientSocket = accept(listeningSoc,(struct sockaddr*) &clientAdderInfo,&addressSize);
	if(toClientSocket < 0)
	{
		printf("Error: failed to accept connection: %s\n", strerror(errno));
		close(listeningSoc);
		return NETWORK_FUNC_FAILURE;
	}

	if (max_client_num >= MAX_CLIENT_NUM)
	{
		//assume working and write ready.
		openning_message invalid_msg;
		create_openning_message_negative(&invalid_msg);
		send(toClientSocket,(char*)(&invalid_msg),1,0);
	}
	else{
		//add new connection to clients_array and append max_client_num
		if (num_of_players >= AWATING_CLIENTS_NUM)
		{
			new_client = create_buff_socket(toClientSocket,SPECTATOR,num_of_players);
			num_of_players++;
		}
		else{
			new_client = create_buff_socket(toClientSocket,PLAYER,num_of_players);
		}
		clients_array[max_client_num] = new_client;
		max_client_num++;
	}

	//send first message- for first byte assume write ready
	create_openning_message(&first_msg,IsMisere,num_of_players,max_client_num-1,new_client->client_stat);
	error = push(new_client->output_buffer,(char*)(&first_msg),sizeof(client_turn_message));
	if (error == NETWORK_FUNC_FAILURE)
	{
		printf("unable to push first message to client %d\n",max_client_num-1);
		return NETWORK_FUNC_FAILURE;
	}

	return 0;
}

//handle all input and output to clients (all the game actually)
int handle_reading_writing(fd_set* read_set,fd_set* write_set,int *prev_client_to_move){
	//error indicator
	int error = 0;
	//abstract message continer
	message_container *abs_message = (message_container*)malloc(sizeof(message_container));
	//game command message
	player_move_msg *game_move = NULL;
	//get pop status
	int pop_stat = MSG_NOT_COMPLETE;
	//who should move this turn
	int curr_to_play = *prev_client_to_move;//in case no move was done,prev_client_to_move will remain the same
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


	//pop all whole commands - and handle chat and quit
	for (int i = 0; i < max_client_num; ++i)
	{
		//if client not alive - don't try anything
		if (clients_array[i] == NULL) continue;

		pop_stat = pop_message(clients_array[i]->input_buffer,abs_message);
		if (pop_stat == MSG_NOT_COMPLETE) continue;
		else if (pop_stat == INVALID_MESSAGE)
		{
			//undefined message
			printf("Errors in reading from input buffers - buffer of client %d",i);
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
			else {
				error = quit_client_handle(i);
			}
		}
	}

	if (game_move != NULL)
	{
		//handle game
		round_reasult = makeRound(game_move->heap_index,game_move->amount_to_remove,&is_leagel_move);
		free(game_move);
		

		if (round_reasult == NONE)//the game continue
		{
			error = send_move_leagelness(curr_to_play,is_leagel_move);
			error = send_heap_mss_to_all(GAME_CONTINUES);
		}
		else {
			error = send_move_leagelness(curr_to_play,is_leagel_move);
			error = send_heap_mss_to_all(GAME_OVER);
			return END_GAME;
		}

		//updated next client to move only when a move was done to move  
		curr_to_play = calc_next_player(*prev_client_to_move);
	}

	//set who was prev to play
	*prev_client_to_move=curr_to_play;
	return 1;
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
		close(listeningSocket);
		return NETWORK_FUNC_FAILURE;
	}

	// listen to connections
	if ( listen(listeningSocket, AWATING_CLIENTS_NUM) < 0){
		printf("Error: listening error: %s\n", strerror(errno));
		close(listeningSocket);
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
	send_buff = (char*) malloc(sizeof(client_to_client_message)+chat_header->length+1);//+1 for NULL
	//fill send_buff
	send_buff[0]=chat_header->message_type;
	send_buff[1]=chat_header->sender_id;
	send_buff[2]=chat_header->destination_id;
	send_buff[3]=chat_header->length;
	error = pop(clients_array[sender]->output_buffer,send_buff+4,chat_header->length);
	if (error == 1)
	{
		printf("error poping client %d chat message\n", sender);
		return NETWORK_FUNC_FAILURE;
	}

	if (chat_header->destination_id == -1)
	{
		//run over all and send
		for (int i = 0; i < max_client_num; ++i)
		{
			error = push(clients_array[i]->output_buffer,send_buff,sizeof(client_to_client_message)+chat_header->length);
			if (error == OVERFLOW_ERROR)
			{
				printf("error pushing chat to client %d\n", i);
				return NETWORK_FUNC_FAILURE;
			}
		}
	}
	else {
		//check dest validity and send to spesicik client
		if (clients_array[(int)(chat_header->destination_id)] != NULL)
		{
			error = push(clients_array[(int)(chat_header->destination_id)]->output_buffer,send_buff,sizeof(client_to_client_message)+chat_header->length);
			if (error == OVERFLOW_ERROR)
			{
				printf("error pushing chat to client %d\n", (int)(chat_header->destination_id));
				return NETWORK_FUNC_FAILURE;
			}
		}
	}

	free(send_buff);
	return 0;
}

int game_message_handle(int curr_to_play,int curr_user,message_container** message_container_p,player_move_msg** game_move_p){
	int error =0;

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
		if (error == OVERFLOW_ERROR)
		{
			printf("error pushing illegal_move to client %d\n", curr_user);
			return NETWORK_FUNC_FAILURE;
		}
		free(illegal_move);
	}
	return 0;
}

int quit_client_handle(int quiting_client){
	int error =0;
	if (clients_array[quiting_client]->client_stat == PLAYER)
	{
		num_of_players--;
		free_buff_socket(clients_array[quiting_client]);
		clients_array[quiting_client] = NULL;//so we will know that the client is dead
		//try to promote player
		for (int j = quiting_client; j < max_client_num; ++j)
		{
			if (clients_array[j] == NULL) continue;
			else if (clients_array[j]->client_stat == PLAYER) continue;

			//promote one SPECTATOR
			clients_array[j]->client_stat = PLAYER;
			promotion_msg *promote_mes = (promotion_msg*)malloc(sizeof(promotion_msg));
			//fill send buff
			create_promotion_message(promote_mes);
			//push message
			error = push(clients_array[j]->output_buffer,(char*)promote_mes,sizeof(promotion_msg));
			if (error == OVERFLOW_ERROR)
			{
				printf("error pushing promote_messsage to client %d\n", j);
				return NETWORK_FUNC_FAILURE;
			}
			free(promote_mes);
		}
	}
	else{
		free_buff_socket(clients_array[quiting_client]);
		clients_array[quiting_client] = NULL;
	}
	return 0;
}

int send_heap_mss_to_all(int is_victory){
	int error =0;

	heap_update_message* heap_mes= (heap_update_message*)malloc(sizeof(heap_update_message));
	create_heap_update_message(heap_mes,heaps_array,is_victory);
	for (int i = 0; i < max_client_num; ++i)
	{
		error = push(clients_array[i]->output_buffer,(char*)heap_mes,sizeof(heap_update_message));
		if (error == OVERFLOW_ERROR)
		{
			printf("error in pushing heap_update_message to client:%d\n",i);
		}
	}
	free(heap_mes);
	return 0;
}
	
int  send_move_leagelness(int curr_to_play,bool is_leagel_move){
	int error =0;

	if (!is_leagel_move)
	{
		illegal_move_message *illegal_move = (illegal_move_message*)malloc(sizeof(illegal_move_message));
		//fill send buff
		create_illegal_move_message(illegal_move);
		//push message
		error = push(clients_array[curr_to_play]->output_buffer,(char*)illegal_move,sizeof(illegal_move_message));
		if (error == OVERFLOW_ERROR)
		{
			printf("can't push leagel_move to %d\n",curr_to_play);
			free(illegal_move);
			return NETWORK_FUNC_FAILURE;
		}
		free(illegal_move);
	}
	else {
		ack_move_message *legal_move = (ack_move_message*)malloc(sizeof(ack_move_message));
		//fill send buff
		create_ack_move_message(legal_move);
		//push message
		error = push(clients_array[curr_to_play]->output_buffer,(char*)legal_move,sizeof(ack_move_message));
		if (error == OVERFLOW_ERROR)
		{
			printf("can't push leagel_move to %d\n",curr_to_play);
			free(legal_move);
			return NETWORK_FUNC_FAILURE;
		}
		free(legal_move);
	}
	return 0;
}

//this function run over all clients and fills the input buffers with information sent by client
int read_to_buffs(fd_set* read_set){
	int error = 0;
	int cur_socket = -1;//the socket we are currently pulling info from recive
	message_container input_msg;
	int resvied_size =0;
	for (int i = 0; i < max_client_num; ++i)
	{
		//if client not alive - don't try anything
		if (clients_array[i] == NULL) continue;

		cur_socket = clients_array[i]->sockfd;
		if (FD_ISSET(cur_socket,read_set)){
			resvied_size = recv(cur_socket,(char*)(&input_msg),sizeof(message_container),0);
			if (resvied_size < 0)
			{
				printf("Error reading from socket of client number %d\n",i );
				return NETWORK_FUNC_FAILURE;
			}
			else if (resvied_size == 0) //in this case - connection broken by user quit
			{
				quit_client_handle(i);
			}
			error = push(clients_array[i]->input_buffer,(char*)(&input_msg),resvied_size);
			if (error == NETWORK_FUNC_FAILURE)
			{
				printf("Error pushing to input buffer of client number %d\n in read_to_buffs",i );
				return NETWORK_FUNC_FAILURE;
			}
		}
	}
	return 0;
}

int send_info(fd_set* write_set){
	int error = 0;
	int cur_socket = -1;//the socket we are currently pulling info from recive
	int resvied_size =0;
	for (int i = 0; i < max_client_num; ++i)
	{
		//if client not alive - don't try anything
		if (clients_array[i] == NULL) continue;

		cur_socket = clients_array[i]->sockfd;
		if (FD_ISSET(cur_socket,write_set)){
			int is_connection_closed =0 ;
			//try to send all the buffer
			resvied_size = send_partially(cur_socket,(char*)(clients_array[i]->output_buffer),clients_array[i]->output_buffer->size,&is_connection_closed);
			if (resvied_size < 0)
			{
				printf("Error sending to socket of client number %d\n",i );
				return NETWORK_FUNC_FAILURE;
			}
			if (is_connection_closed == 1)
			{
				quit_client_handle(i);
			}
			//pop all the information we was able to send
			error =pop_no_return(clients_array[i]->output_buffer,resvied_size);
			if (error == NETWORK_FUNC_FAILURE)
			{
				printf("Error pushing to output buffer the unsent part of client number %d\n in send_info",i );
				return NETWORK_FUNC_FAILURE;
			}
		}
	}
	return 0;
}

void setReadSet(fd_set* read_set,int listeningSoc){
	FD_ZERO(read_set);
	FD_SET(listeningSoc,read_set);
	for (int i = 0; i < max_client_num; ++i)
	{
		//if client not alive - don't try anything
		if (clients_array[i] == NULL) continue;

		FD_SET(clients_array[i]->sockfd,read_set);
	}
}

void setWriteSet(fd_set* write_set){
	FD_ZERO(write_set);
	for (int i = 0; i < max_client_num; ++i)
	{
		//if client not alive - don't try anything
		if (clients_array[i] == NULL) continue;

		FD_SET(clients_array[i]->sockfd,write_set);
	}
}

//this function will find the max socket number. used for select
int findMax(int listeningSoc){
	int max = listeningSoc;
	for (int i = 0; i < max_client_num; ++i)
	{
		//if client not alive - don't try anything
		if (clients_array[i] == NULL) continue;

		if (max < clients_array[i]->sockfd)
		{
			max = clients_array[i]->sockfd;
		}
	}
	return max;
}