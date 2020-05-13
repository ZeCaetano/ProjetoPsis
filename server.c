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
#include "time.h"

#include "UI_library.h"
#include "functions.h"


Uint32 Event_Update;


int dimensions[2];   //dimensions of the board
char **board;        //matrix with positions occupied by pacmans, monsters, fruits, bricks
pthread_t file_thread;
int client_sockets[MAX_CLIENT];     //socket for every client
char_data all_pac[MAX_CLIENT];
char_data all_monster[MAX_CLIENT];



int main(){
    pthread_t connect_thread;
    srand(time(NULL));
    int done = 0;
    SDL_Event event;

    for(int i = 0; i < MAX_CLIENT; i++){
        client_sockets[i] = DISCONNECT;
    }    

    pthread_create(&file_thread, NULL, read_file, NULL);
    pthread_create(&connect_thread, NULL, connect_client, NULL);

    for(int i = 0; i < MAX_CLIENT; i++){
        init_character(&all_pac[i], PACMAN, i, 0, 0, 0);
        init_character(&all_monster[i], MONSTER, i, 0, 0, 0);
    }

    pthread_join(connect_thread, NULL);    

    create_board_window(dimensions[0], dimensions[1]);

    while(!done){
        if(SDL_WaitEvent(&event)){

        }
    }


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
        printf("sending id%d \n", client_id);
        send(client_sockets[client_id], &client_id, sizeof(int), 0);                
        send(client_sockets[client_id], dimensions, (sizeof(int) * 2), 0);        
        pthread_create(&thread_id, NULL, client, (void *)&client_sockets[client_id]);
        client_id ++;
    }
    pthread_exit(NULL);
}


void *client(void *arg){
    int *sock_fd = (int*)arg;
    char_data update;
    char_data previous;
    printf("New client thread created\n");
    int rand_pos[2];

    for(int i = 0; i < 2; i++){
        rand_pos[0] = rand() % dimensions[0];
        rand_pos[1] = rand() % dimensions[1];             //sends random starting positions
        printf("random positions to send: %d %d\n", rand_pos[0], rand_pos[1]);
        send(*sock_fd, rand_pos, (sizeof(int) * 2), 0);
    }
    
    while(1){      
        if(recv(*sock_fd, &update, sizeof(char_data), 0) == 0){  
            printf("Client disconnected \n");
          //  disconnect_player(update.client_id);
          //  update.type = DISCONNECT;
          //  push_update(update, previous_pos);
            break;
        }        
        push_update(update, previous);
        printf("x%d y%d \tid %d type %d\n", update.pos[0], update.pos[1], update.id, update.type);
    }
    pthread_exit(NULL);
}
