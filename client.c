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


static int sockfd;         /* socket to connect to the server, global (private) variable */
static int client_type;    /* holds SPECTATOR or PLAYER */
static int client_turn;    /* 1 iff it is currently this clients turn */

int main(int argc, char* argv[])
{

	char* host_name   = DEFAULT_HOST;
	char* server_port = DEFAULT_PORT;
	
	// process command line parameters
	read_program_input(argc, argv, &host_name, &server_port);
	
	// establish connection to server (sets global sockfd) 
	connect_to_server(host_name, server_port);

	// read and handle openning message from server
	read_openning_message();

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


/* this method reads heaps' sizes from the server and prints them to the user */

void get_heap_sizes()
{
	// first receive the data from the server
	char buffer[HEAP_MESSAGE_SIZE];

	read_server_message(buffer, HEAP_MESSAGE_SIZE);

	// otherwise, we have successfully recieved heaps' sizes, print them
	print_heaps((short*)buffer);

}

/* 
	this method reads a message from the server (reads num_bytes into buffer)
	if connection was closed by the other side, a proper message will be printed (print_closed_connection())
	if any other error occured, a proper message will be printed
	in anycase, the global socket descriptor will be closed and program terminated 
*/
void read_server_message(char* buffer, int num_bytes)
{
	int closed_connection = 0 ; // flag to indicate that the server has closed its connection
	if(recv_all(sockfd, buffer, num_bytes, &closed_connection))
	{
		// error
		if(closed_connection)
		{
			/* other end was closed */
			print_closed_connection();
		}
		else /* different error */
			printf("%s: %s\n", RECV_ERR, strerror(errno));
		
		quit();
	}
}

/* 
	this method sends a message to the server (reads num_bytes into buffer)
	if connection was closed by the other side, a proper message will be printed (print_closed_connection())
	if any other error occured, a proper message will be printed
	in anycase, the global socket descriptor will be closed and program terminated 
*/

void send_message(char* buffer, int num_bytes)
{

	int closed_connection = 0;
	if(send_all(sockfd, buffer, num_bytes, &closed_connection))
	{
		// error 
		if(closed_connection)
		{
			/* other end was closed */
			print_closed_connection();
		}
		else
		{
			printf("%s: %s\n", SEND_ERR, &closed_connection);
		}

		quit();
	

	}
}


/* 
   protocol implementation part 
   this method communicates with the server to play nim
   sockfd should hold a valid socket descriptor to communicate with the server
*/


void play_nim()
{
	
	/* main client loop */
	for(;;)
	{
		// construct fd_set for reading from sockets
		fd_set read_set;
		FD_ZERO(&read_set);

		// add stdin, sockfd
		FD_SET(sockfd, &read_set);
		FD_SET(STDIN_FILENO, &read_set);

		int num_ready;

		if((num_ready = select( sockfd + 1, &read_set, NULL, NULL, NULL)) < 0)
		{
			printf("%s %s", SELECT_ERR, strerror(errno));
			quit();
		}


		// handle server message
		if(FD_ISSET(sockfd, &read_set))
		{
			handle_server_message();
		}

		// handle user input
		if(FD_ISSET(STDIN_FILENO, &read_set))
		{
			handle_user_input();
		}

		

	}
	
	
}

/*
	this method handles user input, figures what the user wants to do (send message or make a move)
	and handles the errors along the way 
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
	if(scanf("%2s", &buffer) != 2)
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
	
	/* finally, send the message */
	client_to_client_message message;
	create_client_to_client_message(&message, client_id, destination_id, (char)count);

	send_message(&message, sizeof(message));  /* send header  */
	send_message(buffer_msg, count);          /* send content */
	

}



/*
	this method handles receives a message from the server, classifiying and handling it
*/
void handle_server_message()
{
	
	message_container message;
	int connection_closed;
	int err_code = read_message(sockfd, &message, &connection_closed)
	if(err_code != SUCCESS)
	{
		handle_receive_error(err_code, connection_closed);
	}

	/* otherwise, read message from server successfully */
	switch(message.message_type)
	{
	case HEAP_UPDATE_MSG:
		handle_heaps_update((*heap_update_message)(&message));
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
		handle_player_message((*player_to_player_message)(&message));
		break;
	case PROMOTION_MSG:
		client_type = PLAYER; /* client is not playing */
		print_promotion();
		break;
	default:
		/* cant reach this part*/
		break;
	}
}



/* 
	this method handles a message that was sent from another client 

*/
void handle_player_message(client_to_client_message* msg)
{
	char msg_buffer[MAX_MSG_SIZE + 1];
	unsigned message_size = (unsigned)(msg->length);
	// close string
	msg_buffer[message_size] = 0;

	// receive the actual message
	read_server_message(msg_buffer, message_size);

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

	if(msg->game_over != GAME_OVER)
	{
		// bogus message
		handle_receive_error(INVALID_MESSAGE_HEADER, 0);
	}

	if(client_type == SPECTATOR)
	{
		print_game_over();
		return;
	}

	/* else, read another byte from the server, that holds whether we won or lost */
	char winning_status;
	read_server_message(&winning_status, 1);

	if(winning_status != WIN && winning_status != LOSE)
	{
		// bogus message
		handle_receive_error(INVALID_MESSAGE_HEADER, 0);
	}

	print_game_over(winning_status);
	quit();
	
}




/*
	this method reads the number of items to remove from heap, then sends a request to the server to make a move
	remove those items from given heap.
	if client is a spectator, an error message will be printed and the game will continue
	if any (other) error occures (sending error, input error) a proper message is printed and method is terminated
*/

void handle_user_move(char heap)
{

	// calculate heap number to update (0 index is the left most)
	char heap_num  = req - 'A';
	
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
	// build request for server and send it
	player_move_msg msg; 

	create_player_move_message(&msg, heap_num, items_to_remove);

	send_message(&msg, sizeof(msg));

	client_turn = 0; /* the turn passed */


}

/* this method frees resources and exits the program */
void quit()
{
	close_socket(sockfd);
	exit(0);
}


void read_openning_message()
{
	int connection_closed = 0 ;
	openning_message msg;
	int ret;

	ret = read_openning_message(sockfd, &msg, &connection_closed);
	
	if(ret != SUCCESS)
	{
		handle_receive_error(ret, connection_closed);
	}

	// else, ret == SUCCESS
	if(msg->connection_accepted == CONNECTION_DENIED)
	{
		print_connection_refused();
		quit();
	}

	// otherwise, connection accepted, process the message

	// check if message is valid
	if(valiadte_opening_message(msg))
	{
		handle_receive_error(INVALID_MESSAGE_HEADER, 0);
	}
	// get and set client type
	client_type = msg.client_type;

	// print the openning message
	proccess_openning_message(msg);

}


void handle_receive_error(int return_code, int connection_closed)
{


	if(return_code == INVALID_MESSAGE_HEADER)
	{
		// not a connection error, but rather the server sent garbage message
		// consider the connection closed
		printf(INVALID_MSG_ERR);
	}

	else if(connection_closed)
	{
		/* other end was closed */
		print_closed_connection();
	}
	else
	{
		/* some other recv error, use errno */
		printf("%s: %s\n", RECV_ERROR, strerror(errno));
	}

	quit();

}