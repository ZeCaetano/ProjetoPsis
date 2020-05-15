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
int client_sockets[MAX_CLIENT][2];     //socket for every client; [0] for socket, [1] for client it corresponds
char_data all_pac[MAX_CLIENT];
char_data all_monster[MAX_CLIENT];



int main(){
    pthread_t connect_thread;
    srand(time(NULL));
    int done = 0;
    SDL_Event event;

    for(int i = 0; i < MAX_CLIENT; i++){
        client_sockets[i][1] = DISCONNECT;
    }    

    read_file();
    pthread_create(&connect_thread, NULL, connect_client, NULL);

    for(int i = 0; i < MAX_CLIENT; i++){
        init_character(&all_pac[i], PACMAN, DISCONNECT, 0, 0, 0);
        init_character(&all_monster[i], MONSTER, DISCONNECT, 0, 0, 0);
    }

   // pthread_join(connect_thread, NULL);    
    create_board_window(dimensions[0], dimensions[1]);

    while(!done){
        if(SDL_WaitEvent(&event)){
            if(event.type == SDL_QUIT){
                done = SDL_TRUE;
            }
            if(event.type == Event_Update){
                char_data *data = event.user.data1;
                char_data *previous = event.user.data2;
                paint_update(data, previous, all_pac, all_monster);                        
                free(previous);                 
                free(data);   
            }
        }
    }
    close_board_windows();


    for(int i = 0; i < dimensions[1]; i++){
        free(board[i]);
    }
    free(board);
    return(0);
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
    printf("Ready to accept connections\n");

    while(1){
        client_sockets[client_id][0] = accept(server_socket_fd, (struct sockaddr *)&client_addr[client_id], &len_client_addr);
        if(client_sockets[client_id][0] == -1){
            perror("accept");
            exit(-1);
        }
        printf("Connection made with client\n");
        printf("sending id%d \n", client_id);
        client_sockets[client_id][1] = client_id;
                
        pthread_create(&thread_id, NULL, client, (void *)client_sockets[client_id]);
        client_id ++;
    }
    pthread_exit(NULL);
}


void *client(void *arg){
    int *sock = (int*)arg;
    int color[3];
    char_data update;
    char_data previous;
    printf("New client thread created\n");
    int rand_pos[2];

    send(sock[0], &sock[1], sizeof(int), 0);                
    send(sock[0], dimensions, (sizeof(int) * 2), 0);    
    recv(sock[0], color, (sizeof(int) * 3), 0);
   
    init_character(&all_pac[sock[1]], PACMAN, sock[1], color[0], color[1], color[2]);
    init_character(&all_monster[sock[1]], MONSTER, sock[1], color[0], color[1], color[2]);
    init_character(&previous, 0, 0, 0, 0, 0);
    for(int i = 0; i < dimensions[1]; i++){
        send(sock[0], board[i], (sizeof(char)*dimensions[0]), 0);
        for(int j = 0; j < dimensions[0]; j++){
            printf("%c", board[i][j]);
        }
        printf("\n");
    }

    for(int i = 0; i < 2; i++){
        rand_pos[0] = rand() % dimensions[0];
        rand_pos[1] = rand() % dimensions[1];             //sends random starting positions
        printf("random positions to send: %d %d\n", rand_pos[0], rand_pos[1]);
     //   send(sock[0], rand_pos, (sizeof(int) * 2), 0);
        if(i == 0){
            all_pac[sock[1]].pos[0] = rand_pos[0];
            all_pac[sock[1]].pos[1] = rand_pos[1];              
            push_update(all_pac[sock[1]], previous);          
        }
        else{
            all_monster[sock[1]].pos[0] = rand_pos[0];
            all_monster[sock[1]].pos[1] = rand_pos[1];
            push_update(all_monster[sock[1]], previous);            
        }                
    }
    
    send(sock[0], all_pac, (sizeof(char_data) * MAX_CLIENT), 0);
    send(sock[0], all_monster, (sizeof(char_data) * MAX_CLIENT), 0);
    send_update(all_pac[sock[1]]);
    send_update(all_monster[sock[1]]);  //sends to all other clients the newly connected player
    
    while(1){      
        if(recv(sock[0], &update, sizeof(char_data), 0) == 0){  
            disconnect_player(update.id);
            printf("Client disconnected \n");
            update.type = DISCONNECT;
            push_update(update, previous);
            send_update(all_pac[update.id]);
            break;
        }                
        if(update.type == PACMAN){
            if(valid_movement(update, all_pac)){
                previous = all_pac[update.id];
                all_pac[update.id] = update;
                push_update(all_pac[update.id], previous);
                send_update(all_pac[update.id]);
            }            
        }
        else if(update.type == MONSTER){
            if(valid_movement(update, all_monster)){
                previous = all_monster[update.id];
                all_monster[update.id] = update;
                push_update(all_monster[update.id], previous);
                send_update(all_monster[update.id]);
            }
        }        
        
       //printf("x%d y%d \tid %d type %d\n", update.pos[0], update.pos[1], update.id, update.type);
    }
    pthread_exit(NULL);
}


int valid_movement(char_data update, char_data character[MAX_CLIENT]){
    int ret = 0;
    if(update.pos[0] != character[update.id].pos[0] || update.pos[1] != character[update.id].pos[1]){  //if it moved
        if((update.pos[0] == character[update.id].pos[0] + 1 && update.pos[1] == character[update.id].pos[1])  
        || (update.pos[0] == character[update.id].pos[0] - 1 && update.pos[1] == character[update.id].pos[1]) 
        || (update.pos[1] == character[update.id].pos[1] + 1 && update.pos[0] == character[update.id].pos[0])
        || (update.pos[1] == character[update.id].pos[1] - 1 && update.pos[0] == character[update.id].pos[0])){
            ret = 1;
            }
    }
    return ret;
}


void send_update(char_data update){
    for(int i = 0; i < MAX_CLIENT; i++){
        if(client_sockets[i][1] != DISCONNECT){  //to send to just the conected players
            send(client_sockets[i][0], &update, sizeof(char_data), 0);
        }                                               
    }
}

void disconnect_player(int id){
    all_pac[id].id = id;
    all_pac[id].type = DISCONNECT;
    client_sockets[id][1] = DISCONNECT;
    for(int i = 0; i < MAX_CLIENT; i++){
        if(client_sockets[i][1] != DISCONNECT){
            printf("sending disconect\n");
            send(client_sockets[i][0], &all_pac[id], sizeof(char_data), 0);
        }
    }
}                      