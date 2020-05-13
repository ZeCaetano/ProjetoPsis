#include <stdio.h>
#include <stdlib.h>

#define PORT 55555
#define MAX_CLIENT 10
//general functions


//malloc with the if condition to check if it succeeded
void *checked_malloc(size_t size);

//server specific functions

//function that opens and reads the board.txt witht the information about the dimensions and bricks
void *read_file(void *arg);  
void *connect_client(void *arg);


//player specific funtions

//connects to the server
void connect_server(char *argv[], int *sock_fd);