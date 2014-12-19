/* This unit is resposible for printing various game related information to the console for the user
   such as, titles, winner id, heap represtantion, etc
*/

/* NOTE: documantation for each method can be found at client_game_tools.c */
void print_game_type(unsigned char isMisere);
void print_title(void);
void print_winner(unsigned char game_status);
void print_message_acked(unsigned char game_status);
void print_turn_message(void);
void print_heaps(short* heaps);
void print_closed_connection(void);