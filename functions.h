#include <stdio.h>
#include <stdlib.h>

//general functions


//malloc with the if condition to check if it succeeded
void *checked_malloc(size_t size);

//server specific functions

//function that opens and reads the board.txt witht the information about the dimensions and bricks
void read_file();  