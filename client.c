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
#define RECV_ERR          "Error: Failed to recieve data from server"


static int sockfd; /* socket to connect to the server, global (private) variable */

int main(int argc, char* argv[])
{

	char* host_name =   DEFAULT_HOST;
	char* server_port = DEFAULT_PORT;
	
	// process command line parameters
	read_program_input(argc, argv, &host_name, &server_port);
	
	// establish connection to server (sets global sockfd) 
	connect_to_server(host_name, server_port);
	
	// run the game protocol
	play_nim();

}

/* 
	method reads arguments given in command line and parses them
	*host_name, *server_port will be modified is non default value was given.
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
   protocol implementation part 
   this method communicates with the server to play nim
   sockfd should hold a valid socket descriptor to communicate with the server
*/


void play_nim()
{
	/* first byte of protocol */
	char game_type;

	/* print nim title */
	print_title();

	/* read the openning message, isMisere, one byte*/
	read_server_message(&game_type, sizeof(char));
	
	/* print the game type message to the console */
	print_game_type(game_type);

	
	/* first byte of each message recieved from the server (except the first unique message that only describes game type)
		 contains game status flags as described in the protocol
	 */
	unsigned char game_status;	 
	int first = 1;	/* simple flag to indiciate whether it is the first time we're printing the heaps */

	do
	{
		if(first)
		{
			first = 0;
		}
		else
		{
			// also print if last message was accepted or not
			print_message_acked(game_status);
		}

		// get heaps' sizes from the server, and print them to the user
		get_heap_sizes();
		
		// read user input and act accordingly (send query if input is not Q)
		handle_user_move();

		// recieve status byte from server (one byte)
		read_server_message((char*)(&game_status), sizeof(char));

	} while(!hasGameEnded(game_status));

	/* if we have reached this part, game has ended, print the last messages */
	print_message_acked(game_status);
	get_heap_sizes();
	print_winner(game_status);
	return;
	
}



/*
	this method reads heap number (starts from 0, meaning left most) and number of items to remove from heap
	method then sends this request (following the protocol) to the server
	if any error occures (sending error, input error) a proper message is printed and method is terminated
*/

void handle_user_move()
{
	char req; /* A, B, C, D or Q */

	print_turn_message();

	if(scanf("%1s", &req) != 1)
	{
		printf(USER_INPUT_ERR);
		quit();
	}
	
	if(req == 'Q')
	{
		quit();
	}
	// calculate heap number to update (0 index is the left most)
	unsigned char heap_num = 0;
	if(req == 'A' || req == 'B' || req == 'C' || req == 'D')
	{
		heap_num = (unsigned char)(req - 'A');
	}
	else
	{
		// error, invalid input
		printf(USER_INPUT_ERR);
		quit();
	}
	
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
		

	// build request for server: heap number (byte), next one short: items_to_remove (network byte order)
	char client_query[CLIENT_QUERY_SIZE];
	client_query[0] = heap_num;
	unsigned short* p = (unsigned short*)(client_query + 1);
	p[0] = htons(items_to_remove);

	int connection_closed;

	// send the query
	if(send_all(sockfd, client_query, CLIENT_QUERY_SIZE, &connection_closed))
	{
		// error
		if(connection_closed)
		{
			/* other end was closed. */
			print_closed_connection();
		}
		else {
			printf("%s: %s\n", SEND_ERR, strerror(errno));
		}
		quit();
		
	}
	return;

}

/* this method frees resources and exits the program */
void quit()
{
	close_socket(sockfd);
	exit(0);
}
