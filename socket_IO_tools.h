/* this unit provides useful methods for handling socket IO */
#ifndef SOCKET_IO_H
#define SOCKET_IO_H
#define SERVER_MESSAGE_SIZE 9

/* 
	this method tries to receive all the data given (*num_bytes)
	
	if the method failed, a positive value will be returned, on success 0 will be returned.
	note that method might fail if connection on the other end was closed, in this case
	*connection_closed will contain 1 (otherwise, 0)
*/

int recv_all(int sockfd,  char* buffer, int num_bytes, int* connection_closed);

/* 
   method tries to send num_bytes from buffer, via sockfd
   returns 0 on success, or 1 if could not send all the data
   
   if the other end was closed (EPIPE error), *connection_closed will contain 1 (otherwise 0)
   NOTE: send is called with MSG_NOSIGNAL, meaning, no signal will be recieved by the process
   use *connection_closed to determine that the connection was closed by other end 
   
   
*/
int send_all(int sockfd,  char* buffer, int num_bytes, int* connection_closed);


/* 
   method tries to send num_bytes from buffer, via sockfd
   returns the actual number of bytes sent, or a negative value on error
  
   if the other end was closed (EPIPE error), *connection_closed will contain 1 (otherwise 0)
   NOTE: send is called with MSG_NOSIGNAL, meaning, no signal will be recieved by the process
   use *connection_closed to determine that the connection was closed by other end 
   
   
*/
int send_partially(int sockfd, char* buffer, int num_bytes, int* connection_closed);



/* 
	this method tries to receive num_bytes to buffer via sockfd
	returns the number of bytes actually read
	if reading fails [including closed connection], method returns a negative value

	note that method might fail if connection on the other end was closed, in this case
	*connection_closed will contain 1 (otherwise, 0)
*/

int recv_partially(int sockfd, char* buffer, int num_bytes, int* connection_closed);



/* 
	method tries to close given socket, prints messsage on error 
*/
void close_socket(int sockfd);


#endif