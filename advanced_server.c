#include "advanced_server.h"

client_list clients_linked_list;
int max_players_game = 3;//should be given as parameter 3 is for testing
int num_exsisting_players =0;
int min_free_num =0;


int main( int argc, const char* argv[] ){
	short port = 6325;	//set the port to default
	int listeningSoc; //listening socket on port 
	short M;			//initial heaps size
	bool input_isMisere;//initial the kind of game(if the last one to finish the last heap is the winner or the loser)
	struct buffered_socket *running = NULL;

	//check the validity of the argument given by the user.
	if (!checkServerArgsValidity(argc ,argv)){
		printf("Error: invalid arguments given to server\n");
		return 0;
	}

	// read the input from command line
	max_players_game= atoi(argv[1]);
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
		printf("listening socket failed on init\n");
		return 0;
	}

	//init linke list
	init_client_list(&clients_linked_list);

	server_loop(listeningSoc);

	//free all clients_array
	for(running=clients_linked_list.first;running != NULL;running=running->next_client)
	{
		free_buff_socket(running);
	}
}

//this function holds the server live loop
int server_loop(int listeningSoc){
	//thous are the "select variables"
	fd_set read_set;
	fd_set write_set;
	int fd_max=-1;
	//we will use this number to determine which player should do the next move
	int next_to_move = 0;
	//game_stat will report errors or end game
	int game_stat = GAME_CONTINUES;
	int error =0;
	bool first_move = true;

	while(1){
		//prepare for select
		setReadSet(&read_set,listeningSoc);
		setWriteSet(&write_set);
		fd_max = findMax(listeningSoc);

		error = select(fd_max+1,&read_set,&write_set,NULL,NULL);
		if (error < 0)
		{
			printf("error in select ");
			return NETWORK_FUNC_FAILURE;
		}

		//check if there is a new requests to connect - and add it
		if (FD_ISSET(listeningSoc,&read_set))
		{
			printf("new client\n");
			game_stat = get_new_connections(listeningSoc);
			if (game_stat == NETWORK_FUNC_FAILURE)
			{
				printf("Error: could not accept new connection: %s", strerror(errno));
				return NETWORK_FUNC_FAILURE;
			}
		}

		//get all client input and handle it
		game_stat =handle_reading_writing(&read_set,&write_set,&next_to_move,&first_move);
		if (game_stat == NETWORK_FUNC_FAILURE)
		{
			printf("Error in handle_reading_writing function");
			return NETWORK_FUNC_FAILURE;
		}
		else if (game_stat == END_GAME)
		{
			printf("The game has ended. Existing peacefully");
			send_msg_affter_endGame();
			return 0;
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
	if (clients_linked_list.last != NULL && clients_linked_list.last->client_id >= MAX_CLIENTS)
	{
		int is_connection_closed = 0;
		//assume working and write ready
		openning_message invalid_message;
		create_openning_message_negative(&invalid_message);
		if (send_partially(toClientSocket,(char*)(&invalid_message),1,&is_connection_closed))
		{
			if (is_connection_closed)
			{
				//rejected client closed himself, all is good
				close(toClientSocket);
				return 0;
			}
			else{
				//close message sent, no struct made, so just close
				printf("new client didn't close connection. we kick him off\n");
				close(toClientSocket);
				return 0;
			}
		}
		else{
			printf("Error: failed to reject user in socket %d\n",toClientSocket );
			return NETWORK_FUNC_FAILURE;
		}
	}
	else if(num_exsisting_players >= max_players_game){
		printf("new SPECTATOR number %d\n", min_free_num);
		new_client = create_buff_socket(toClientSocket,SPECTATOR,min_free_num);
		add_client(&clients_linked_list,new_client);
	}
	else{
		
		new_client = create_buff_socket(toClientSocket,PLAYER,min_free_num);
		printf("new PLAYER number %d\n",min_free_num);
		add_client(&clients_linked_list,new_client);
		num_exsisting_players++;
	}

	//send first message- for first byte assume write ready
	create_openning_message(&first_msg,IsMisere,num_exsisting_players,min_free_num,new_client->client_stat);
	//printf("%d\n",(((char*)(&first_msg))[0]) );
	error = push(new_client->output_buffer,(char*)(&first_msg),sizeof(openning_message));
	if (error == NETWORK_FUNC_FAILURE)
	{
		printf("unable to push first message to client %d\n",min_free_num);
		return NETWORK_FUNC_FAILURE;
	}
	printf("new_client->output_buffer->size %d\n",new_client->output_buffer->size );
	calc_new_min_by_occupy();// update min_free_num
	return 0;
}

//handle all input and output to clients (all the game actually)
int handle_reading_writing(fd_set* read_set,fd_set* write_set,int *curr_to_play,bool *first_move){
	buffered_socket *running = NULL;
	//error indicator
	int error = 0;
	//abstract message continer
	message_container *abs_message = (message_container*)malloc(sizeof(message_container));
	//game command message
	player_move_msg *game_move = NULL;
	//get pop status
	int pop_stat = MSG_NOT_COMPLETE;
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
	for(running=clients_linked_list.first;running != NULL;running=running->next_client)
	{
		pop_stat = pop_message(running->input_buffer,abs_message);
		//PROBLEM 1- if running->input_buffer->size  == 0 return INVALID_MESSAGE and not MSG_NOT_COMPLETE
		if (pop_stat == MSG_NOT_COMPLETE) {continue;}
		else if (pop_stat == INVALID_MESSAGE)
		{
			//undefined message
			printf("Errors in reading from input buffers - buffer of client %d\n",running->client_id);
			return NETWORK_FUNC_FAILURE;
		}
		else { //must be pop_stat == SUCCESS
			if (abs_message->message_type == PLAYER_MOVE_MSG)
			{
				error = game_message_handle(*curr_to_play,running->client_id,&abs_message,&game_move);
			}
			else if (abs_message->message_type == MSG){//
				error = chat_message_handle(running->client_id,abs_message);
			}
			else { //error in message type
				printf("starnge error - error in message type\n");
				return NETWORK_FUNC_FAILURE;
			}
		}
	}
	//we got all the messages and handled them (exsept for the game). abs_message no longer needed
	free(abs_message);

	if (game_move != NULL)
	{
		//handle game
		round_reasult = makeRound(game_move->heap_index,game_move->amount_to_remove,&is_leagel_move);
		free(game_move);
		

		if (round_reasult == NONE)//the game continue
		{
			error = send_move_leagelness(*curr_to_play,is_leagel_move);

			if (error == NETWORK_FUNC_FAILURE)
			{
				printf("error in sending is move leagel\n");
				return NETWORK_FUNC_FAILURE;
			}

			error = send_heap_mss_to_all(GAME_CONTINUES);

			if (error == NETWORK_FUNC_FAILURE)
			{
				printf("error in sending heaps messages\n");
				return NETWORK_FUNC_FAILURE;
			}
		}
		else {
			error = send_move_leagelness(*curr_to_play,is_leagel_move);

			if (error == NETWORK_FUNC_FAILURE)
			{
				printf("error in sending is move leagel\n");
				return NETWORK_FUNC_FAILURE;
			}

			error = send_heap_mss_to_all(GAME_OVER);

			if (error == NETWORK_FUNC_FAILURE)
			{
				printf("error in sending heaps messages\n");
				return NETWORK_FUNC_FAILURE;
			}
			return END_GAME;
		}

		//updated next client to move only when a move was done to move  
		*curr_to_play = calc_next_player(*curr_to_play);
		//TODO : send to the next player- "you should play"
		error = send_your_move(*curr_to_play);
		if (error == NETWORK_FUNC_FAILURE)
		{
			printf("error in sending heaps messages\n");
			return NETWORK_FUNC_FAILURE;
		}
	}
	else if (*first_move)
	{
		error = send_your_move(*curr_to_play);
		if (error == NETWORK_FUNC_FAILURE)
		{
			printf("error in sending heaps messages\n");
			return NETWORK_FUNC_FAILURE;
		}

		*first_move = false;
	}

	return GAME_CONTINUES;
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
	if ( listen(listeningSocket, MAX_CLIENTS+1) < 0){
		printf("Error: listening error: %s\n", strerror(errno));
		close(listeningSocket);
		return NETWORK_FUNC_FAILURE;
	}
	return listeningSocket;
}

int calc_next_player(int prev_player){
	buffered_socket* prev_sock = get_buffered_socket_by_id(prev_player);
	//all the first p once are players, so if we came to the list end or SPECTATOR next to move is the list head
	if (prev_sock->next_client == NULL || prev_sock->next_client->client_stat == SPECTATOR)
	{
		return clients_linked_list.first->client_id;
	}
	else{
		return prev_sock->next_client->client_id;
	}
}

buffered_socket* get_buffered_socket_by_id(int req_id){
	buffered_socket *running = NULL;
	for(running=clients_linked_list.first;running != NULL;running=running->next_client){
		if (running->client_id == req_id)
		{
			return running;
		}
	}
	return NULL;//should never get here
}

int chat_message_handle(int sender,message_container *abs_message){
	buffered_socket *running = NULL;
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
	error = pop(get_buffered_socket_by_id(sender)->output_buffer,send_buff+4,chat_header->length);
	if (error == 1)
	{
		printf("error poping client %d chat message\n", sender);
		return NETWORK_FUNC_FAILURE;
	}

	if (chat_header->destination_id == -1)
	{
		//run over all and send
		for(running=clients_linked_list.first;running != NULL;running=running->next_client)
		{
			error = push(running->output_buffer,send_buff,sizeof(client_to_client_message)+chat_header->length);
			if (error == OVERFLOW_ERROR)
			{
				printf("error pushing chat to client %d\n",running->client_id);
				return NETWORK_FUNC_FAILURE;
			}
		}
	}
	else {
		//check dest validity and send to spesicik client
		if (get_buffered_socket_by_id(sender) != NULL)
		{
			error = push(get_buffered_socket_by_id(sender)->output_buffer,send_buff,sizeof(client_to_client_message)+chat_header->length);
			if (error == OVERFLOW_ERROR)
			{
				printf("error pushing chat to client %d\n", get_buffered_socket_by_id(sender)->client_id);
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
		illegal_move_message illegal_move;
		//fill send buff
		create_illegal_move_message(&illegal_move);
		//push message
		error = push(get_buffered_socket_by_id(curr_user)->output_buffer,(char*)(&illegal_move),sizeof(illegal_move_message));
		if (error == OVERFLOW_ERROR)
		{
			printf("error pushing illegal_move to client %d\n", curr_user);
			return NETWORK_FUNC_FAILURE;
		}
	}
	return 0;
}

int quit_client_handle(int quiting_client){
	buffered_socket *running = NULL;
	int error =0;

	calc_min_by_new_free(quiting_client);//calc new min_free_num if quiting_client<min_free_num

	if (get_buffered_socket_by_id(quiting_client)->client_stat == PLAYER)
	{
		num_exsisting_players--;
		delete_by_client_id(&clients_linked_list,quiting_client);

		//try to promote player
		for (running = get_buffered_socket_by_id(quiting_client); running != NULL; running = running->next_client)
		{
			promotion_msg promote_mes;

			if (running->client_stat == PLAYER) continue;

			//promote one SPECTATOR
			running->client_stat = PLAYER;
			//fill send buff
			create_promotion_message(&promote_mes);
			//push message
			error = push(running->output_buffer,(char*)(&promote_mes),sizeof(promotion_msg));
			if (error == OVERFLOW_ERROR)
			{
				printf("error pushing promote_messsage to client %d\n", running->client_id);
				return NETWORK_FUNC_FAILURE;
			}
			//one SPECTATOR promoted to PLAYER
			num_exsisting_players++;
			return 0;
		}
	}
	else{
		delete_by_client_id(&clients_linked_list,quiting_client);
	}
	return 0;
}

int send_heap_mss_to_all(int is_victory){
	buffered_socket *running = NULL;
	int error =0;

	heap_update_message heap_mes;
	create_heap_update_message(&heap_mes,heaps_array,is_victory);
	for(running=clients_linked_list.first;running != NULL;running=running->next_client)
	{
		error = push(running->output_buffer,(char*)(&heap_mes),sizeof(heap_update_message));
		if (error == OVERFLOW_ERROR)
		{
			printf("error in pushing heap_update_message to client:%d\n",running->client_id);
		}
	}
	return 0;
}
	
int  send_move_leagelness(int curr_to_play,bool is_leagel_move){
	int error =0;

	if (!is_leagel_move)
	{
		illegal_move_message illegal_move;
		//fill send buff
		create_illegal_move_message(&illegal_move);
		//push message
		error = push(get_buffered_socket_by_id(curr_to_play)->output_buffer,(char*)(&illegal_move),sizeof(illegal_move_message));
		if (error == OVERFLOW_ERROR)
		{
			printf("can't push leagel_move to %d\n",curr_to_play);
			return NETWORK_FUNC_FAILURE;
		}
	}
	else {
		ack_move_message legal_move;
		//fill send buff
		create_ack_move_message(&legal_move);
		//push message
		error = push(get_buffered_socket_by_id(curr_to_play)->output_buffer,(char*)(&legal_move),sizeof(ack_move_message));
		if (error == OVERFLOW_ERROR)
		{
			printf("can't push leagel_move to %d\n",curr_to_play);
			return NETWORK_FUNC_FAILURE;
		}
	}
	return 0;
}

//this function run over all clients and fills the input buffers with information sent by client
int read_to_buffs(fd_set* read_set){
	buffered_socket *running = NULL;
	buffered_socket *running_next = NULL;
	int error = 0;
	int cur_socket = -1;//the socket we are currently pulling info from recive
	message_container input_msg;
	int resvied_size =0;

	for(running=clients_linked_list.first;running != NULL;)
	{
		cur_socket = running->sockfd;
		if (FD_ISSET(cur_socket,read_set)){
			resvied_size = recv(cur_socket,(char*)(&input_msg),sizeof(message_container),0);
			if (resvied_size < 0)
			{
				printf("Error reading from socket of client number %d\n",running->client_id );
				return NETWORK_FUNC_FAILURE;
			}
			else if (resvied_size == 0) //in this case - connection broken by user quit
			{
				running_next = running->next_client;
				quit_client_handle(running->client_id);
				running = running_next;
			}
			error = push(running->input_buffer,(char*)(&input_msg),resvied_size);
			if (error == NETWORK_FUNC_FAILURE)
			{
				printf("Error pushing to input buffer of client number %d\n in read_to_buffs",running->client_id );
				return NETWORK_FUNC_FAILURE;
			}
		}
		//advance loop
		running=running->next_client;
	}
	return 0;
}

int send_info(fd_set* write_set){
	buffered_socket *running_next = NULL;
	buffered_socket *running = NULL;
	int error = 0;
	int cur_socket = -1;//the socket we are currently pulling info from recive
	int resvied_size =0;

	for(running=clients_linked_list.first;running != NULL;)
	{
		cur_socket = running->sockfd;
		if (FD_ISSET(cur_socket,write_set)){
			int is_connection_closed =0 ;
			//try to send all the buffer
			resvied_size = send_partially(cur_socket,(char*)(running->output_buffer->buffer),running->output_buffer->size,&is_connection_closed);
			if (resvied_size < 0)
			{
				printf("Error sending to socket of client number %d\n",running->client_id  );
				return NETWORK_FUNC_FAILURE;
			}
			if (is_connection_closed == 1)
			{
				running_next = running->next_client;
				printf("quiting client\n");
				quit_client_handle(running->client_id);
				running = running_next;
			}
			if (resvied_size >0 )
			{
				printf("to client %d sent %d bytes\n", running->client_id,resvied_size);
			}
			//pop all the information we was able to send
			error =pop_no_return(running->output_buffer,resvied_size);
			if (error == NETWORK_FUNC_FAILURE)
			{
				printf("Error pushing to output buffer the unsent part of client number %d\n in send_info",running->client_id );
				return NETWORK_FUNC_FAILURE;
			}
		}
		//advance loop
		running=running->next_client;
	}
	return 0;
}

void setReadSet(fd_set* read_set,int listeningSoc){
	buffered_socket *running = NULL;

	FD_ZERO(read_set);
	if (listeningSoc!= -1)
	{
		FD_SET(listeningSoc,read_set);
	}
	for(running=clients_linked_list.first;running != NULL;running=running->next_client)
	{
		FD_SET(running->sockfd,read_set);
	}
}

void setWriteSet(fd_set* write_set){
	buffered_socket *running = NULL;

	FD_ZERO(write_set);
	for(running=clients_linked_list.first;running != NULL;running=running->next_client)
	{
		FD_SET(running->sockfd,write_set);
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

int send_msg_affter_endGame(){
	//thous are the "select variables"
	fd_set read_set;
	fd_set write_set;
	int fd_max=-1;
	int error =0;
	buffered_socket *running = NULL;
	buffered_socket *running_next = NULL;

	while(1){
		bool finished_sending = true;

		setReadSet(&read_set,-1);
		setWriteSet(&write_set);
		fd_max = findMax(-1);//we doesn't try to read listening socket now

		error = select(fd_max+1,&read_set,&write_set,NULL,NULL);
		if (error <0 )
		{
			printf("error in select ");
			return NETWORK_FUNC_FAILURE;
		}

		printf("sending rest of the game\n");
		//check if all buffers are empty and check who quited
		for(running=clients_linked_list.first;running != NULL;)
		{
			if (FD_ISSET((running->sockfd),&write_set))
			{
				//assume in this case it means connection closed
				running_next = running->next_client;
				quit_client_handle(running->client_id);
				running = running_next;//this is because now running points to junk
				continue;
			}
			if (running->output_buffer->size > 0)
			{
				finished_sending = false;
			}
			//advance loop
			running=running->next_client;
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
	}
}

void calc_min_by_new_free(int new_free){
	if (new_free<min_free_num)
	{
		min_free_num = new_free;
	}
}

void calc_new_min_by_occupy(){
	buffered_socket *running = NULL;
	int is_num_occupied[MAX_CLIENTS]={0};
	//check which numbers are occupied
	for(running=clients_linked_list.first;running != NULL;running=running->next_client)
	{
		is_num_occupied[running->client_id] = 1;
	}

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (is_num_occupied[i] == 0)
		{
			min_free_num = i;
			return;
		}
	}
}

/*  
	this method checks whether the given arguments given in command line are valid
	returns true if are valid, returns false otherwise
*/ 
#define MAX_HEAP_SIZE  1500
bool checkServerArgsValidity(int argc,const char* argv[]){

	// check if the number of arguments is valid
	if(argc > 5 || argc < 4) return false;

	errno = 0;
	//check p parameter

	long p = strtol(argv[2], NULL, 10);
	if((p == LONG_MIN || p == LONG_MAX) && errno != 0)
		// overflow or underflow
		return false;
	if( p == 0 )
		return false;
	
	if (p < 1 || p > MAX_CLIENTS) 
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
	if (strlen(argv[2]) > 1)
		return false;
	if(argc == 5)
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

int send_your_move(int next_player){
	int error =0;
	printf("send your_turn_move to client %d\n",next_player);
	client_turn_message your_turn_move;
	//fill send buff
	create_client_turn_message(&your_turn_move);
	//push message
	error = push(get_buffered_socket_by_id(next_player)->output_buffer,(char*)(&your_turn_move),sizeof(client_turn_message));
	if (error == OVERFLOW_ERROR)
	{
		printf("can't push client_turn_message to %d\n",next_player);
		return NETWORK_FUNC_FAILURE;
	}
	printf("end send_your_move\n");
	return 0;
}
