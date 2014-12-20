
/* this unit defines protocol messages sizes, and provides various methods for the client and server to 
   create and handle protocol messages */

#define NUM_HEAPS           4
#define MAX_MSG_SIZE        255
#define MAX_HEAP_SIZE       1500

/* define openning message constants */

#define CONNECTION_ACCEPTED 1
#define CONNECTION_DENIED   0

#define MISERE              1
#define REGULAR             0

#define SPECTATOR           0
#define PLAYER              1

#define GAME_OVER           1  /* used in heap update message */
#define GAME_CONTINUES      0  /* used in heap update message */

#define WIN                 1  /* client won */
#define LOSE                0  /* client lost */


/* define message types */

#define HEAP_UPDATE_MSG     1


#define CLIENT_TURN_MSG     2

#define ACK_MOVE_MSG        3
#define ILLEGAL_MOVE_MSG    4

#define MSG                 5 

#define PROMOTION_MSG       6

#define PLAYER_MOVE_MSG     7


#pragma pack(push, 1)

/*
	struct represents the first opening message that a server sends to a new client
*/


typedef struct openning_message
{
	char connection_accepted;         /* CONNECTION_ACCEPTED or CONNECTION_DENIED */
	char isMisere;                    /* MISERE or REGULAR */
	char p;                           /* number of players <= 9 */
	char client_id;                   /* client id <= 24 */
	char client_type;                 /* PLAYER or SPECTATOR */

} openning_message;

#pragma pack(pop)


#pragma pack(push, 1)

/*
	represents a message that a server sends to the client in order 
	to notify him about the amount of items in each heap
	each such message also contains a flag that indicates whether the game 
	has ended or continues
*/

typedef struct heap_update_message
{
	char message_type;              /* HEAP_UPDATE_MSG     */
	short heaps[4];                 /* heap representation */
	char game_over;                 /* GAME_OVER or GAME_CONTINUES */

} heap_update_message;

#pragma pack(pop)

/*

	represents a message that a server sends to a player client 
	to notify that it is his turn
*/

#pragma pack(push, 1)
typedef struct client_turn_message
{
	char message_type;               /* CLIENT_TURN_MSG */
} client_turn_message
#pragma pack(pop)


/*
	represents a message that a server sends to a player in order to ack him last move
	(to notify that his last move was valid)
*/
#pragma pack(push, 1)
typedef struct ack_move_message
{
	char message_type;              /* ACK_MOVE_MSG */
} ack_move_message;
#pragma pack(pop)

/*
	represents a message that a server sends to a player in order to notify
	him that his sent move was illegal 
*/

#pragma pack(push, 1)
typedef struct illegal_move_message
{
	char message_type;              /* ILLEGAL_MOVE_MSG */
} illegal_move_message;
#pragma pack(pop)


/*
	represents a message that is passed between the clients, through the server
	this struct only contains the protocol header and not the message itself
*/
#pragma pack(push, 1)
typedef struct client_to_client_message
{
	char message_type;              /* MSG */
	char sender_id;                 /* id the sender */
	char destination_id;            /* id of destination */
	char length;                    /* length of the (content of) message in bytes <= MAX_MSG_SIZE */

} client_to_client_message;
#pragma pack(pop)


/*
	represents a message that is sent by the server to a specific client that the server wants to promote from a spectator to a player
*/
#pragma pack(push, 1)
typedef struct promotion_msg
{
	char message_type;              /* PROMOTION_MSG */

} promotion_msg;
#pragma pack(pop)

/*
	represents a message that is sent from the client to the server in order to make a move in the nim game
*/
#pragma pack(push, 1)
typedef struct player_move_msg
{
	char message_type;              /* PLAYER_MOVE_MSG */
	char heap_index;                /* index of heap to remove items from , starts with 0 */
	short amount_to_remove;  /* amount of items to remove from the selected heap <= MAX_HEAP_SIZE (positive value)*/

} player_move_msg;
#pragma pack(pop)


/*
	represents a container big enough to hold any (valid) message passed between the server and the client
	the idea is similar to sock_addr struct
	note that this struct should NOT be passed on the network, but rather exist on the end hosts
	also note that this struct should not be used for the openning message recieved by the client
*/
#pragma pack(push, 1)

#define MAX_FILLER_SIZE 10          /* the biggest message is the heap update message */

typedef struct message_container
{
	char message_type;              /* message type as defined above */
	char filler[10];                /* message container */

} message_container;
#pragma pack(pop)



/* get message size in bytes according to its type, used in the method below */
int get_message_size(int message_type);

/* method that reads a message from the client or from the server, according to the protocol 
   used for message structs that have message type field ( basiclly all messages except openning_message )
*/
int read_message(int sockfd, message_container* container, int* connection_closed);

/*
  method that reads the openning message from the server, used in the client only 
*/
int read_openning_message(int sockfd, openning_message* msg, int* connection_closed )


/* constants for the return values of read_message and read_openning_message */

#define INVALID_MESSAGE_HEADER 1
#define CONNECTION_ERROR       2
#define SUCCESS                0