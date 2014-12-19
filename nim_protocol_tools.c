#include "nim_protocol_tools.h"

/* this unit defines protocol messages sizes, and provides various methods to the container byte
   a single byte that contains various game status flags, that is passed in the beginning of each
   server response to the client (except the first message to the client), as defined in the protocol
*/

/* number of bit shifts required to get the flags */
#define ACK_NUM_SHIFTS 0
#define GAME_OVER_NUM_SHIFTS 1
#define SERVER_WON_NUM_SHIFTS 2

/* return information from the given container (byte) */
int hasGameEnded(unsigned char container)
{
	container >>= GAME_OVER_NUM_SHIFTS;
	return container & 1;
}

int lastMessageAcked(unsigned char container)
{
	container >>= ACK_NUM_SHIFTS;
	return container & 1;
}

int hasServerWon(unsigned char container)
{
	container >>= SERVER_WON_NUM_SHIFTS;
	return container & 1;
}


/* reset container
   by default, the flags are set to:
		game continues
		last client query was not acked
		
*/
void init_container(unsigned char* container)
{
	/* no ack, game continues */
	*container = 0;
}

/* accept client's last message */
void ackClientMessage(unsigned char* container)
{
	unsigned char flag = 1 << ACK_NUM_SHIFTS;
	(*container) |= flag;
}
void setServerWon(unsigned char* container)
{
	unsigned char flag = 1 << SERVER_WON_NUM_SHIFTS;
	(*container) |= flag;
	setGameOver(container);
}
void setClientWon(unsigned char* container)
{
	/* by default, 3rd bit is 0 indiciating that client won
	   all we need to do is set the game over bit to 1 */
	setGameOver(container);
}

void setGameOver(unsigned char* container)
{
	unsigned char flag = 1 << GAME_OVER_NUM_SHIFTS;
	(*container) |= flag;

}