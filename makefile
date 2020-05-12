#Compiler
CC = gcc

#Flags
WFLAG = -Wall
SDFLAG = -lSDL2 -lSDL2_image
THREADFLAG = -lpthread
ALLFLAGS = -Wall -lSDL2 -lSDL2_image -lpthread

#other C files to linkage
SDL = UI_library

all: clean player server
#player: player
#server: server

#clean
SERVER = server
$(SERVER): $(SERVER).c
	$(CC) -g -o $(SERVER) $(SERVER).c $(SDL).c $(ALLFLAGS)

PLAYER = player
$(PLAYER): $(PLAYER).c
	$(CC) -g -o $(PLAYER) $(PLAYER).c $(SDL).c $(ALLFLAGS)

clean:
	rm $(SERVER) $(PLAYER) 
