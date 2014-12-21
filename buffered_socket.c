#include "buffered_socket.h"



/* 
	this method allocates a new buffered socket
	handles and prints errors

	if a malloc error occured, frees resources, prints error and returns NULL
	else, the allocated object is returned 

*/

buffered_socket* create_buff_socket(int sockfd)
{
	buffered_socket* res = (buffered_socket*)malloc(sizeof(buffered_socket));
	if( res == NULL)
	{
		printf("Error: malloc failed to allocate memory for buffered socket: %d\n", sockfd);
		return NULL;
	}

	res->sockfd = sockfd;
	res->input_buffer = (io_buffer*)malloc(sizeof(io_buffer));
	if(res->input_buffer == NULL)
	{
		printf("Error: malloc failed to allocate memory for input buffer, of socket %d\n", sockfd);
		free(res);
		return NULL;
	}

	res->output_buffer = (io_buffer*)malloc(sizeof(io_buffer));
	if(res->output_buffer == NULL)
	{
		printf("Error: malloc failed to allocate memory for output buffer, of socket %d\n", sockfd);
		free(res->input_buffer);
		free(res);
		return NULL;
	}

	return res;

}

/*
	free all resources allocated by a given buffered socket 
*/


void free_buff_socket(buffered_socket* sock)
{
	free(sock->input_buffer);
	free(sock->output_buffer);
	free(sock);
}




