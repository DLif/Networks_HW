FLAGS= -Wall -g -lm -std=gnu99 -pedantic-errors

all: nim nim-server

clean:
	-rm nim nim-server client.o client_game_tools.o nim_protocol_tools.o socket_IO_tools.o nim_game.o game_server.o

nim: client.o client_game_tools.o nim_protocol_tools.o socket_IO_tools.o
	gcc -o $@ $^ $(FLAGS)

nim-server: game_server.o socket_IO_tools.o nim_protocol_tools.o nim_game.o
	gcc -o $@ $^ $(FLAGS)

client.o: client.c client.h client_game_tools.h nim_protocol_tools.h socket_IO_tools.h
	gcc -c $*.c $(FLAGS)

client_game_tools.o: client_game_tools.c client_game_tools.h nim_protocol_tools.h
	gcc -c $*.c $(FLAGS)

nim_protocol_tools.o: nim_protocol_tools.c nim_protocol_tools.h
	gcc -c $*.c $(FLAGS)

socket_IO_tools.o: socket_IO_tools.c socket_IO_tools.h
	gcc -c $*.c $(FLAGS)

game_server.o: game_server.c game_server.h socket_IO_tools.h nim_protocol_tools.h nim_game.h
	gcc -c $*.c $(FLAGS)

nim_game.o: nim_game.c nim_game.h
	gcc -c $*.c $(FLAGS)
