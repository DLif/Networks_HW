FLAGS= -Wall -g -lm -std=gnu99 -pedantic-errors

all: nim-server nim

clean:
	-rm nim nim-server client.o client_game_tools.o nim_protocol_tools.o socket_IO_tools.o nim_game.o advanced_server.o

nim: client.o client_game_tools.o nim_protocol_tools.o socket_IO_tools.o buffered_socket.o IO_buffer.o 
	gcc -o $@ $^ $(FLAGS)

nim-server: advanced_server.o buffered_socket.o nim_protocol_tools.o IO_buffer.o nim_game.o socket_IO_tools.o
	gcc -o $@ $^ $(FLAGS)

client.o: client.c client.h client_game_tools.h nim_protocol_tools.h socket_IO_tools.h buffered_socket.h IO_buffer.h
	gcc -c $*.c $(FLAGS)

client_game_tools.o: client_game_tools.c client_game_tools.h nim_protocol_tools.h
	gcc -c $*.c $(FLAGS)

nim_protocol_tools.o: nim_protocol_tools.c nim_protocol_tools.h
	gcc -c $*.c $(FLAGS)

socket_IO_tools.o: socket_IO_tools.c socket_IO_tools.h
	gcc -c $*.c $(FLAGS)

advanced_server.o: advanced_server.c advanced_server.h buffered_socket.h nim_protocol_tools.h IO_buffer.h nim_game.h
	gcc -c $*.c $(FLAGS)

nim_game.o: nim_game.c nim_game.h
	gcc -c $*.c $(FLAGS)

IO_buffer.o: IO_buffer.c IO_buffer.h nim_protocol_tools.h
	gcc -c $*.c $(FLAGS)

buffered_socket.o: buffered_socket.c buffered_socket.h IO_buffer.h
	gcc -c $*.c $(FLAGS)

#client_list.o: client_list.c client_list.h buffered_socket.h nim_protocol_tools.h IO_buffer.h
#	gcc -c $*.c $(FLAGS)