/* this unit is resposible for establishing connection with the server, 
   implementing the protocol of communication with the server, 
   and providing the user the abillity to play nim 
*/


int main(int argc, char* argv[]);


/* 
	method reads arguments given in command line and parses them
	*host_name, *server_port will be modified i non default value was given.
	on invalid input: error message will be printed and program will be terminated.
*/

void read_program_input(int argc, char* argv[], char** host_name, char** server_port);


/* method connects to given server (host_name : server_port)
   on error: fitting message is printed and program quits 
   on success: global variable sockfd is set to the server connection socket descriptor
*/

void connect_to_server(const char* host_name, const char* server_port);




/* 
   protocol implementation part 
   this method communicates with the server to play nim
   
*/


void play_nim();


/*
	this method handles user input, figures what the user wants to do (send message or make a move)
	and handles the errors along the way 
	if everything went smooth, the desired message will be put onto the sockets output buffer
*/

void handle_user_input();




/*
	this method handles a message from the sockets input buffer, classifiying and handling it
	(the message is taken for the input buffer )

	returns MSG_NOT_COMPLETE is message is not complete and cant be handled
	quits if message is not valid 
	returns SUCCESS if a message was successfully handled
*/
int handle_server_message();


/* 
	this method handles a message that was sent from another client 

*/
void handle_player_message(client_to_client_message* msg);




/*
	This method handles a heap update message from the server
	also handles if game has ended, and receives winning status if not spectator 
*/

void handle_heaps_update(heap_update_message* msg);


/*
	this method reads the number of items to remove from heap, then creates a request to the server to make a move
	remove those items from given heap, then stores this message onto the socket's output buffer.
	if client is a spectator, an error message will be printed and the game will continue
	if any (other) error occures (sending error, input error) a proper message is printed and program is terminated
*/

void handle_user_move(char heap);


/* this method frees resources and exits the program */
void quit();


/**

	this method recieves the openning message from the server,
	prints the required information
	and initalizes the required data structures 
**/

void handle_openning_message();

/**

	simple methods to handle receive/write errors or connection closure 
**/


void handle_receive_error( int connection_closed);


void handle_send_error( int connection_closed);
