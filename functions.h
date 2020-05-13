#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define PORT 55555
#define MAX_CLIENT 10
#define PACMAN 0
#define MONSTER 1
#define DISCONNECT -111

typedef struct char_data{  //character data
    int pos[2];            //position
    int color[3];          //color, rgb
    int type;              //pacman or monster
    int id;                 //id given by server
} char_data;


//general functions


//malloc with the if condition to check if it succeeded
void *checked_malloc(size_t size);
//initializes the struct for client data
void init_character(char_data *character, int type, int id, int r, int g, int b);
//pushes the updated move to the sdl queue
void push_update(char_data update, char_data previous);

//server specific functions

//function that opens and reads the board.txt witht the information about the dimensions and bricks
void read_file();
//inializes sockets and connects to the client  
void *connect_client(void *arg);  
//receives updates from client, analyses it and pushes it to the SDL queue
void *client(void *arg);
int valid_movement(char_data update, char_data character[MAX_CLIENT]);



//player specific funtions

//connects to the server
void connect_server(char *argv[], int *sock_fd);