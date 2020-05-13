#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "UI_library.h"
#include "functions.h"

int dimensions[2];   //dimensions of the board
char **board;        //matrix with positions occupied by pacmans, monsters, fruits, bricks

int main(){

    read_file();
    
    for(int i = 0; i < dimensions[1]; i++){
        for(int j = 0; j < dimensions[0]; j++){
            printf("%c ", board[i][j]);
        }
        printf("\n");
    }


    for(int i = 0; i < dimensions[1]; i++){
        free(board[i]);
    }
    free(board);
    
}


void read_file(){
    FILE *fp = NULL;
    fp = fopen("board.txt", "r");
    char c = '\0';
    
    if(fp == NULL){
        printf("File error\n");
        exit(0);
    }

    fscanf(fp, "%d %d", &dimensions[0], &dimensions[1]);    //reads the dimensions from the file
    printf("%d %d\n", dimensions[0], dimensions[1]);
    c = fgetc(fp);      //to read the \n on the first line
    board = checked_malloc(sizeof(char*) * dimensions[1]);             //rows
    for(int i = 0; i < dimensions[1]; i++){
        int j = 0;
        board[i] = checked_malloc(sizeof(char) * dimensions[0]);      //columns
        while((c = fgetc(fp)) != '\n'){
            board[i][j] = c;
            j++;
        }        
    }

   
    
    fclose(fp);
}


void *checked_malloc(size_t size){
    void *ptr;
    ptr = malloc (size);
    if(ptr == NULL){
        printf("Malloc error\n");
        exit(0);
    }
    return ptr;

}