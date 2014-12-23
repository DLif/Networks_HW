/* this unit provides useful methods for handling socket IO and sockets in general*/
#include "socket_IO_tools.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


/* 
	this method tries to receive all the data given (*num_bytes)
	
	if the method failed, a positive value will be returned, on success 0 will be returned.
	note that method might fail if connection on the other end was closed, in this case
	*connection_closed will contain 1 (otherwise, 0)
*/

int recv_all(int sockfd, char* buffer, int num_bytes, int* connection_closed)
{
	int temp;
	int total_recv = 0;
	int left_to_recv = num_bytes;

	while(left_to_recv > 0)
	{
		temp = recv(sockfd, buffer + total_recv, left_to_recv, 0);
		if(temp <= 0) // other side closed connection
			break;
		total_recv += temp;
		left_to_recv -= temp;
	}

	
	/* check if connection was closed, and set the given argument accordingly */
	
	*connection_closed = (temp == 0 ? 1 : 0);

	/* if we didn't complete the task return 1, otherwise return 0 */
	return (left_to_recv > 0 ? 1: 0);
}



/* 
	this method tries to receive num_bytes to buffer via sockfd
	returns the number of bytes actually read
	if reading fails [including closed connection], method returns a negative value

	note that method might fail if connection on the other end was closed, in this case
	*connection_closed will contain 1 (otherwise, 0)
*/

int recv_partially(int sockfd, char* buffer, int num_bytes, int* connection_closed)
{
	int temp;

	temp = recv(sockfd, buffer , num_bytes, 0);

	/* check if connection was closed, and set the given argument accordingly */
	if( temp == 0)
	{
		*connection_closed = 1;
		return -1;
	}

	*connection_closed = 0;

	/* return num of bytes actually read  or error */
	return temp; 
}

/* 
	method tries to send num_bytes from buffer, via sockfd
   returns 0 on success, or 1 if could not send all the data
  
   if the other end was closed (EPIPE error), *connection_closed will contain 1 (otherwise 0)
   NOTE: send is called with MSG_NOSIGNAL, meaning, no signal will be recieved by the process
   use *connection_closed to determine that the connection was closed by other end 
   
   
*/
int send_all(int sockfd, char* buffer, int num_bytes, int* connection_closed)
{
	int num_sent, total_sent = 0; 
	int bytes_left = num_bytes;

	while(bytes_left > 0) {
		/* send without recieved sigpipe signal */
		num_sent = send(sockfd, buffer + total_sent, bytes_left, MSG_NOSIGNAL);
		if(num_sent == -1)
		{
			// failed to send
			break;
		}
		total_sent += num_sent;
		bytes_left -= num_sent;
	}
	

	if(num_sent == -1)
	{
		// some error occured, might be that the connection was closed by other end
		*connection_closed = (errno == EPIPE ? 1 : 0);
		return 1;
	}

	return 0;

	
}


/* 
   method tries to send num_bytes from buffer, via sockfd
   returns the actual number of bytes sent, or a negative value on error
  
   if the other end was closed (EPIPE error), *connection_closed will contain 1 (otherwise 0)
   NOTE: send is called with MSG_NOSIGNAL, meaning, no signal will be recieved by the process
   use *connection_closed to determine that the connection was closed by other end 
   
   
*/
int send_partially(int sockfd, char* buffer, int num_bytes, int* connection_closed)
{
	

		/* send without recieved sigpipe signal */
	int	num_sent = send(sockfd, buffer , num_bytes, MSG_NOSIGNAL);
	

	if(num_sent == -1)
	{
		// some error occured, might be that the connection was closed by other end
		*connection_closed = (errno == EPIPE ? 1 : 0);
		return -1;
	}

	return num_sent;

	
}

/* 
	method tries to close given socket, prints messsage on error 
*/
#define CLOSE_SOCKET_ERR "Error while closing socket"
void close_socket(int sockfd)
{
	if(close(sockfd))
	{
		printf("%s: %s", CLOSE_SOCKET_ERR, strerror(errno));
	}
	
}


