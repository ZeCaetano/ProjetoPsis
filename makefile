#Compiler
CC = gcc

#Flags
WFLAG = -Wall
SDFLAG = -lSDL2 -lSDL2_image
THREADFLAG = -lpthread
ALLFLAGS = -Wall -lSDL2 -lSDL2_image -lpthread

#other C files to linkage
SDL = UI_library
GLOBAL = functions
all: clean player server

SERVER = server
$(SERVER): $(SERVER).c
	$(CC) -g -o $(SERVER) $(SERVER).c $(GLOBAL).c $(SDL).c $(ALLFLAGS)

PLAYER = player
$(PLAYER): $(PLAYER).c
	$(CC) -g -o $(PLAYER) $(PLAYER).c $(GLOBAL).c $(SDL).c $(ALLFLAGS)

clean:
	rm $(SERVER) $(PLAYER)

ADDRS = 192.168.1.5
PORT = 55555
init:
	./server
play:
	./player $(ADDRS) $(PORT) 255 0 0 
