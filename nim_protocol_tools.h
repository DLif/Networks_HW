
/* this unit defines protocol messages sizes, and provides various methods to the container byte
   a single byte that contains various game status flags, that is passed in the beginning of each
   server response to the client (except the first message to the client), as defined in the protocol*/

#define CLIENT_QUERY_SIZE    (1 + sizeof(short))	   /* query sent to the server at client's turn, format:
														  first byte: number of heap (0 is the left most, 3 is the right most)
												          1 short : amount to remove from heap
											            */
#define HEAP_MESSAGE_SIZE   (4*sizeof(short))			/* size of the message from server that describes the heaps' sizes (4 shorts) */
                               

/* container - single byte that is passed in the beginning of each (except the first) message to the client (from server)*/
/* use and set flags inside the container */
void setGameOver(unsigned char* container);
void setClientWon(unsigned char* container);
void setServerWon(unsigned char* container);
void ackClientMessage(unsigned char* container);
void init_container(unsigned char* container);

/* these methods returns positive values if answer is true, 0 otherwise */
/* get flags' values from the container */
int hasServerWon(unsigned char container);
int lastMessageAcked(unsigned char container);
int hasGameEnded(unsigned char container);