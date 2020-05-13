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
pthread_t file_thread;
int client_sockets[MAX_CLIENT];     //socket for every client

int main(){
    pthread_t connect_thread;

    pthread_create(&file_thread, NULL, read_file, NULL);
    pthread_create(&connect_thread, NULL, connect_client, NULL);
    pthread_join(connect_thread, NULL);    

    for(int i = 0; i < dimensions[1]; i++){
        free(board[i]);
    }
    free(board);
    return(0);
}


void *read_file(void *arg){
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
    pthread_exit(NULL);
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


void *connect_client(void *arg){
    int err = 0;
    int server_socket_fd;
    int client_id = 0;
    pthread_t thread_id;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr[MAX_CLIENT];

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    socklen_t len_server_addr = sizeof(server_addr); 
    socklen_t len_client_addr = sizeof(client_addr); 

    server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket_fd == -1){
        perror("socket");
        exit(-1);
    }

    err = bind(server_socket_fd,(struct sockaddr *)&server_addr, len_server_addr);
    if(err == -1) {
		perror("bind");
		exit(-1);
	}

    err = listen(server_socket_fd, 10);
    if(err == -1){
        perror("socket");
        exit(-1);
    }
    pthread_join(file_thread, NULL);       //if for some reason the file is taking long, it will until it's finished
    printf("Ready to accept connections\n");    
    while(1){
        client_sockets[client_id] = accept(server_socket_fd, (struct sockaddr *)&client_addr[client_id], &len_client_addr);
        if(client_sockets[client_id] == -1){
            perror("accept");
            exit(-1);
        }
        printf("Connection made with client\n");
        send(client_sockets[client_id], &client_id, sizeof(int), 0);                
        send(client_sockets[client_id], dimensions, (sizeof(int) * 2), 0);        
       // pthread_create(&thread_id, NULL, client_thread, (void *)&sockets[client_id]);
        client_id ++;
    }
    pthread_exit(NULL);
}

