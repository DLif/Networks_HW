/* this unit is resposible for establishing connection with the server, 
   implementing the protocol of communication with the server, 
   and providing the user the abillity to play nim 
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/ioctl.h>


#include "nim_protocol_tools.h"
#include "socket_IO_tools.h"
#include "client_game_tools.h"
#include "client.h"
#include "buffered_socket.h"
#include "IO_buffer.h"

#define DEFAULT_HOST    "localhost"
#define DEFAULT_PORT    "6325"


/* various messages used in this file */
#define INPUT_ERR         "Error: invalid arguments\n"
#define ADDR_ERR          "Error: could not retrieve server IPv4 address\n"
#define SOCKET_CREATE_ERR "Error: Failed to create socket"
#define CONNECT_ERR       "Error: Failed to connect to server"
#define USER_INPUT_ERR    "Error: invalid arguments given by user\n"
#define SEND_ERR          "Error: Failed to send data to server"
#define RECV_ERR          "Error: Failed to receive data from server"
#define INVALID_MSG_ERR   "Error: invalid message received from the server, exiting ... \n"
#define SELECT_ERR        "Error: select failed, "


static int sockfd;                            /* socket to connect to the server, global (private) variable */
static buffered_socket* buff_socket = NULL;   /* buffered socket to connect to the server                   */
static int client_type;                       /* holds SPECTATOR or PLAYER */
static int client_turn;                       /* 1 iff it is currently this clients turn */
static unsigned char client_id;

int main(int argc, char* argv[])
{

	char* host_name   = DEFAULT_HOST;
	char* server_port = DEFAULT_PORT;
	
	// process command line parameters
	read_program_input(argc, argv, &host_name, &server_port);
	
	// establish connection to server (sets global sockfd) 
	connect_to_server(host_name, server_port);

	// read and handle openning message from server
	handle_openning_message();

	// connection was accepted, and client was accepted, run the game protocol
	play_nim();

}

/* 
	method reads arguments given in command line and parses them
	*host_name, *server_port will be modified i non default value was given.
	on invalid input: error message will be printed and program will be terminated.
*/

void read_program_input(int argc, char* argv[], char** host_name, char** server_port)
{
	if(argc > 1)
	{
		
		// host given
		*host_name = argv[1];
		
		if(argc == 3)
		{
			// both host and port were given
			*server_port = argv[2];
		}
		else if (argc > 3)
		{
			printf(INPUT_ERR);
			exit(0);
		}
	}
	
}


/* method connects to given server (host_name : server_port)
   on error: fitting message is printed and program quits 
   on success: global variable sockfd is set to the server connection socket descriptor
*/

void connect_to_server(const char* host_name, const char* server_port)
{
	/* lets connect to the nim server */
	struct addrinfo hints;
	struct addrinfo* server_info; /* will hold getaddrinfo result */
	int err_code;
	
	memset(&hints, 0, sizeof(hints));

	/* we require an IPv4 TCP socket */
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	/* fetch address of given host_name with given server_port */
	if((err_code = getaddrinfo(host_name, server_port, &hints, &server_info)))
	{
		printf("%s: %s\n", ADDR_ERR, gai_strerror(err_code));
		exit(0);
	}

	/* we may connect to the first result */
	/* create socket */
	sockfd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
	if(sockfd  == -1)
	{
		printf("%s: %s\n", SOCKET_CREATE_ERR, strerror(errno));
		freeaddrinfo(server_info);
		exit(0);
	}

	/* connect to server */
	if(connect(sockfd, server_info->ai_addr, server_info->ai_addrlen) == -1)
	{
		/* failed to connect to server, free resources */
		printf("%s: %s\n", CONNECT_ERR, strerror(errno));
		freeaddrinfo(server_info);
		quit();

	}
	
	/* free server_info, only used for establishing connection */
	freeaddrinfo(server_info);
	return;
}







/* 
   protocol implementation part 
   this method communicates with the server to play nim
   
*/


void play_nim()
{
	char buffer[MAX_IO_BUFFER_SIZE]; 
	int connection_closed = 0 ;
	int num_bytes;

	/* main client loop */
	for(;;)
	{
		// construct fd_set for reading from sockets
		fd_set read_set;
		// construct fd_set for writing
		fd_set write_set; 

		FD_ZERO(&read_set);
		FD_ZERO(&write_set);

		// add stdin, sockfd
		FD_SET(sockfd, &read_set);
		FD_SET(STDIN_FILENO, &read_set);
		FD_SET(sockfd, &write_set);

		int num_ready;

		if((num_ready = select( sockfd + 1, &read_set, &write_set, NULL, NULL)) < 0)
		{
			printf("%s %s", SELECT_ERR, strerror(errno));
			quit();
		}


	


		// handle user input and store request message onto the socket's output buffer (if such is created)
		if(FD_ISSET(STDIN_FILENO, &read_set))
		{
			handle_user_input();
		}

		// read from server socket and push onto input stack
		if(FD_ISSET(sockfd, &read_set))
		{
			/* read as much as you can from the socket */
			num_bytes = recv_partially(sockfd, buffer, MAX_IO_BUFFER_SIZE, &connection_closed);
			if(num_bytes < 0 )
			{	
				handle_receive_error(connection_closed);
			}
			/* try to push it onto the socket message buffer */
			if(push(buff_socket->input_buffer, buffer, num_bytes))
			{
				/* input buffer is full */
				printf("Error: client input buffer size limit reached\n");
				quit();
			}

                        /* see if have a server message that we can handle */
		        handle_server_message();
				
		}

		// see if we can write into the server's socket
		if(FD_ISSET(sockfd, &write_set))
		{
			// write from the output's buffer
			num_bytes = send_partially(sockfd, (char*)(buff_socket->output_buffer->buffer), buff_socket->output_buffer->size, &connection_closed);
			if(num_bytes < 0)
			{
				handle_send_error(connection_closed);
			}

			// pop num_bytes bytes from the buffer
			pop_no_return(buff_socket->output_buffer, num_bytes);
		}

	}
	
	
}

/*
	this method handles user input, figures what the user wants to do (send message or make a move)
	and handles the errors along the way 
	if everything went smooth, the desired message will be put onto the socket output buffer
*/

void handle_user_input()
{

	char req; /* A, B, C, D, Q or M */

	if(scanf("%1s", &req) != 1)
	{
		printf(USER_INPUT_ERR);
		quit();
	}
	
	if(req == 'Q')
	{
		/* quit the program */
		quit();
	}

	if(req >= 'A' && req <= 'D')
	{
		/* user wants to make a move, read the rest of the message */
		handle_user_move(req);
		return;
	}
	
	/* other option: MSG */

	if(req != 'M')
	{
		printf(USER_INPUT_ERR);
		quit();
	}

	char buffer[10];
	if(scanf("%2s", buffer) != 2)
	{
		printf(USER_INPUT_ERR);
		quit();
	}

	/* close string */
	buffer[2] = 0;

	if(strcmp(buffer, "SG"))
	{
		// invalid input
		printf(USER_INPUT_ERR);
		quit();
	}

	char destination_id;
	long temp; 

	if(scanf("%ld", &temp) != 1)
	{
		printf(USER_INPUT_ERR);
		quit();
	}

	// determine if the number can be cast to unsigned char properly
	if(temp == LONG_MIN || temp == LONG_MAX || temp == 0 || temp > UCHAR_MAX || temp < -1)
	{
		/* note that we allow temp == -1 */
		printf(USER_INPUT_ERR);
		quit();
	}
	
	// check if broadcast
	if( temp == -1)
	{
		destination_id = -1;
	}
	else
		destination_id = (char)temp;

	/* read the actual message */
	char buffer_msg[MAX_MSG_SIZE];
	int c ; 
	int count = 1;
	int success = 0;
	while((c = getchar()) != -1 && count < MAX_MSG_SIZE)
	{
		if(c != '\n')
			buffer_msg[count-1] = c;
		else
		{
			success = 1;
			break;
		}
	}

	if(!success)
	{
		if( c == -1)
		{
			// EOF or error
			printf("Error: error occured while reading input from user\n");

		}
		else 
			// no error occured, but we run out of buffer space without seeing '\n'
			printf("Error: entered message exceeds maximum message length, which is 255 chars\n");
		quit();
	}
	
	/* create the message struct */
	client_to_client_message message;
	create_client_to_client_message(&message, (char)client_id, destination_id, (char)count);

	/* try to push it onto the socket output buffer to be sent later */
	if(push(buff_socket->output_buffer, (char*)(&message), sizeof(message)) || push(buff_socket->output_buffer, buffer_msg, count))
	{
		/* output buffer is full */
		printf("Error: socket output buffer size limit reached\n");
		quit();
	}
	
}



/*
	this method handles receives a message from the server, classifiying and handling it
	(the message is taken for the input buffer )

	returns MSG_NOT_COMPLETE is message is not complete and cant be handled
	quits if message is not valid 
	returns SUCCESS if a message was successfully handled
*/
int handle_server_message()
{
	
	message_container message;
	/* pop message from the buffer */
	int err_code = pop_message(buff_socket->input_buffer, &message);
	if(err_code == MSG_NOT_COMPLETE)
	{
		/* nothing to do */
		return MSG_NOT_COMPLETE; 
	}

	if(err_code == INVALID_MESSAGE)
	{
		printf("Error: invalid message received from server\n");
		quit();
	}


	/* otherwise, read message from input buffer successfully */
	switch(message.message_type)
	{
	case HEAP_UPDATE_MSG:
		handle_heaps_update((heap_update_message*)(&message));
		break;
	case CLIENT_TURN_MSG:
		print_turn_message();
		client_turn  = 1;  /* currently it is clients turn */
		break;
	case ACK_MOVE_MSG:
	case ILLEGAL_MOVE_MSG:
		print_message_acked(message.message_type);
		break;
	case MSG:
		handle_player_message((client_to_client_message*)(&message));
		break;
	case PROMOTION_MSG:
		client_type = PLAYER; /* client is not playing */
		print_promotion();
		break;
	default:
		
		// in case, the client recieved an invalid message (that only a server handles)
		printf("Error: invalid message received from server\n");
		quit();

	}

	return SUCCESS;
}



/* 
	this method handles a message that was sent from another client 

*/
void handle_player_message(client_to_client_message* msg)
{
	char msg_buffer[MAX_MSG_SIZE + 1];
	int message_size = (unsigned char)(msg->length);
	// close string
	msg_buffer[message_size] = 0;

	// pop the message from the input buffer
	pop(buff_socket->input_buffer, msg_buffer, message_size);

	// print the message to the client
	printf("%u: %s", msg->sender_id, msg_buffer);

}





/*
	This method handles a heap update message from the server
	also handles if game has ended, and receives winning status if not spectator 
*/

void handle_heaps_update(heap_update_message* msg)
{
	/* print the heaps */
	print_heaps(msg->heaps);

	if(msg->game_over == GAME_CONTINUES)
		return;

	// game ended

	if(client_type == SPECTATOR)
	{
		print_game_over_spectator();
		quit();
	}

	/* else, read another byte from the server, that holds whether we won or lost */
	char winning_status;
	int connection_closed; 

	/* check if input buffer contains the byte */
	if(buff_socket->input_buffer->size > 0)
	{
		pop(buff_socket->input_buffer, &winning_status, 1);
	}
	else
	{
		// gotta read it from the server

		if(recv_partially(sockfd, &winning_status, 1, &connection_closed))
		{
			handle_receive_error(connection_closed);
		}
	}

	if(winning_status != WIN && winning_status != LOSE)
	{
		// bogus message
		printf("Error: expected WIN (%d) or LOSE (%d) but received %d from server\n", WIN, LOSE, winning_status);
		quit();
	}

	print_game_over(winning_status);
	quit();
	
}




/*
	this method reads the number of items to remove from heap, then creates a request to the server to make a move
	remove those items from given heap, then stores this message onto the socket's output buffer.
	if client is a spectator, an error message will be printed and the game will continue
	if any (other) error occures (sending error, input error) a proper message is printed and program is terminated
*/

void handle_user_move(char heap)
{

	// calculate heap number to update (0 index is the left most)
	char heap_num  = heap - 'A';
	
	// read number of items to remove
	unsigned short items_to_remove;
	long temp; 

	if(scanf("%ld", &temp) != 1)
	{
		printf(USER_INPUT_ERR);
		quit();
	}

	// determine if the number can be cast to unsigned short properly
	if(temp == LONG_MIN || temp == LONG_MAX || temp <= 0 || temp > USHRT_MAX)
	{
		printf(USER_INPUT_ERR);
		quit();
	}
	items_to_remove = (unsigned short)temp;
		

	if(!client_turn)
	{
		// not clients turn!
		printf("Move rejected: this is not your turn\n");
		return;

	}
	// build request for server and store it onto the buffer
	player_move_msg msg; 

	create_player_move_message(&msg, heap_num, items_to_remove);

	if(push(buff_socket->output_buffer, (char*)(&msg), sizeof(msg)))
	{
		/* output buffer is full */
		printf("Error: socket output buffer size limit reached\n");
		quit();
	}

	client_turn = 0; /* the turn passed */


}

/* this method frees resources and exits the program */
void quit()
{
	close_socket(sockfd);

	if(buff_socket != NULL)
	{
		free_buff_socket(buff_socket);
	}

	exit(0);
}

/**

	this method recieves the openning message from the server,
	prints the required information
	and initalizes the required data structures 
**/

void handle_openning_message()
{
	int connection_closed = 0 ;
	openning_message msg;
	int ret;

	ret = read_openning_message(sockfd, &msg, &connection_closed);
	
	if(ret != SUCCESS)
	{
		if(ret == INVALID_MESSAGE_HEADER)
		{
			printf("Error: invalid format of received openning message\n");
			quit();
		}
		/* network error or connection closed */
		handle_receive_error(connection_closed);
	}

	// else, ret == SUCCESS
	if(msg.connection_accepted == CONNECTION_DENIED)
	{
		print_connection_refused();
		quit();
	}

	// otherwise, connection accepted, process the message

	// check if message is valid
	if(valiadate_openning_message(&msg))
	{
		printf("Error: invalid format of received openning message\n");
		quit();
	}
	// get and set client type and id
	client_type = msg.client_type;
	client_id   = (unsigned char)msg.client_id;

	// create a bufferd socket for buffering input and output to/from socket
	// since the socket is a server, the type field and client id is irrelevant
	buff_socket = create_buff_socket(sockfd, 0, 0);
	if(buff_socket == NULL)
	{
		// malloc error
		quit();
	}

	// print the openning message
	proccess_openning_message(&msg);

}




void handle_receive_error( int connection_closed)
{


	if(connection_closed)
	{
		/* other end was closed */
		print_closed_connection();
	}
	else
	{
		/* some other recv error, use errno */
		printf("%s: %s\n", RECV_ERR, strerror(errno));
	}

	quit();

}

void handle_send_error( int connection_closed)
{


	
	if(connection_closed)
	{


		while(handle_server_message() != MSG_NOT_COMPLETE) {  
			/* parse all messages untill the point we cant parse anymore messages since we're stuck.
			in this case connection was closed wrongfully and we will print the error
			if some invalid message was on the buffer we will print that error first 
			if valid exit message was in the buffer, we will exit correctly 
			*/
		} 


		/* other end was closed */
		print_closed_connection();
	}
	else
	{
		/* some other send error, use errno */
		printf("%s: %s\n", SEND_ERR, strerror(errno));
	}

	quit();

}