/* This unit is resposible for printing various game related information to the console for the user
   such as, titles, winner id, heap represtantion, etc
*/

/* NOTE: documantation for each method can be found at client_game_tools.c */
void print_game_type( char isMisere);
void print_title(void);
void print_message_acked( int acked );
void print_turn_message(void);
void print_heaps(short* heaps);
void print_closed_connection(void);

/*

	This method checks whether the openning message recieved from the server is legal
	i.e. contains values defined in the protocol 

	returns positive value on error, otherwise returns 0
*/


int valiadate_openning_message(openning_message* msg);


/*
	this method prints the openning message to the client
*/

void proccess_openning_message(openning_message* msg);
void print_promotion();
void print_game_over(int win_status);
void print_game_over();
void print_client_type(char client_type);
void print_client_id(char id);
void print_num_players(char p);
void print_connection_refused();