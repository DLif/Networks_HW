/* this unit is resposible for establishing connection with the server, 
   implementing the protocol of communication with the server, 
   and providing the user the abillity to play nim 
*/

/* NOTE: documentation for each method can be found in client.c */
void read_program_input(int argc, char* argv[], char** host_name, char** server_port);
void connect_to_server(const char* host_name, const char* server_port);
void get_heap_sizes(void);
void read_server_message(char* buffer, int num_bytes);
void play_nim(void);
void handle_user_move(void);
void quit(void);