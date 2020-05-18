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
board_struct **board;        //matrix with positions occupied by pacmans, monsters, fruits, bricks
int client_sockets[MAX_CLIENT][2];     //socket for every client; [0] for socket, [1] for client it corresponds
char_data all_pac[MAX_CLIENT];
char_data all_monster[MAX_CLIENT];
int occupied_places;


int main(){
    pthread_t connect_thread;
    srand(time(NULL));
    int done = 0;
    SDL_Event event;
    occupied_places = 0;

    for(int i = 0; i < MAX_CLIENT; i++){
        client_sockets[i][1] = DISCONNECT;
    }    

    read_file();
    pthread_create(&connect_thread, NULL, connect_client, NULL);

    for(int i = 0; i < MAX_CLIENT; i++){
        init_character(&all_pac[i], PACMAN, i, NOT_CONNECT, 0, 0, 0);
        init_character(&all_monster[i], MONSTER, i, NOT_CONNECT, 0, 0, 0);
    }

   // pthread_join(connect_thread, NULL);    
    create_board_window(dimensions[0], dimensions[1]);

    for(int i = 0; i < dimensions[1]; i++){
        for(int j = 0; j < dimensions[0]; j++){
            if(board[i][j].type == 'B'){
                paint_brick(j, i);
            }
        }
    }

    while(!done){
        if(SDL_WaitEvent(&event)){
            if(event.type == SDL_QUIT){
                all_pac[0].state = ENDGAME;
                send_update(all_pac[0]);
                done = SDL_TRUE;
            }
            if(event.type == Event_Update){
                char_data *data = event.user.data1;
                char_data *previous = event.user.data2;
                printf("going to paint x%d y%d \tid %d type %d state %d\n", data->pos[0], data->pos[1], data->id, data->type, data->state);
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
    board = checked_malloc(sizeof(board_struct*) * dimensions[1]);             //rows
    for(int i = 0; i < dimensions[1]; i++){
        int j = 0;
        board[i] = checked_malloc(sizeof(board_struct) * dimensions[0]);      //columns
        while((c = fgetc(fp)) != '\n'){
            if(c == 'B'){
                occupied_places++;
            }
            board[i][j].type = c;
            board[i][j].id = NOT_CONNECT;
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
        client_sockets[client_id][1] = client_id;                
        pthread_create(&thread_id, NULL, client, (void *)client_sockets[client_id]);
        client_id ++;
    }
    pthread_exit(NULL);
}


void *client(void *arg){
    int *sock = (int*)arg;
    char_data update;
    char_data previous;
    printf("New client thread created\n");
    
    player_data(sock, previous);
        
    while(1){      
        if(recv(sock[0], &update, sizeof(char_data), 0) == 0){  
            disconnect_player(update.id);
            printf("Client disconnected \n");
            update.state = DISCONNECT;
            push_update(update, previous);
            send_update(all_pac[update.id]);
            break;
        }               
        char_data previous_pac = all_pac[update.id];                
        char_data previous_monster = all_monster[update.id];
        if(update.type == PACMAN){
            if(valid_movement(update, all_pac)){
                
                if(bounce_on_walls(update, all_pac) == 0){          //if it bounces on a wall it won't bounce on a brick
                    all_pac[update.id] = update;                    //same goes for bounce on brick
                    bounce_on_brick(update.id, all_pac, previous_pac);                    
                }                         
                board[previous_pac.pos[1]][previous_pac.pos[0]].type = ' ';
                board[previous_pac.pos[1]][previous_pac.pos[0]].id = NOT_CONNECT;   
                character_interactions(update.id, all_pac, previous_pac, previous_monster);
                board[all_pac[update.id].pos[1]][all_pac[update.id].pos[0]].type = 'P';
                board[all_pac[update.id].pos[1]][all_pac[update.id].pos[0]].id = update.id;                  
                push_update(all_pac[update.id], previous_pac);
                send_update(all_pac[update.id]);                
            }            
        }
        else if(update.type == MONSTER){
            if(valid_movement(update, all_monster)){                                
                if(bounce_on_walls(update, all_monster) == 0){
                    all_monster[update.id] = update;    
                    bounce_on_brick(update.id, all_monster, previous_monster);                    
                }
                board[previous_monster.pos[1]][previous_monster.pos[0]].type = ' ';
                board[previous_monster.pos[1]][previous_monster.pos[0]].id = NOT_CONNECT;
                character_interactions(update.id, all_monster, previous_pac, previous_monster);
                board[all_monster[update.id].pos[1]][all_monster[update.id].pos[0]].type = 'M';
                board[all_monster[update.id].pos[1]][all_monster[update.id].pos[0]].id = update.id;   
                push_update(all_monster[update.id], previous_monster);
                send_update(all_monster[update.id]);                
            }
        }        
   
      /*  for(int i = 0; i < dimensions[1]; i++){
            for(int j = 0; j < dimensions[0]; j++){
                printf("%c.%d", board[i][j].type, board[i][j].id);
            }
            printf("\n");
        } */         
       //printf("x%d y%d \tid %d type %d\n", update.pos[0], update.pos[1], update.id, update.type);
    }
  //  free(sock);
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
    all_pac[id].state = DISCONNECT;
    board[all_pac[id].pos[1]][all_pac[id].pos[0]].type = ' ';
    board[all_pac[id].pos[1]][all_pac[id].pos[0]].id = DISCONNECT;
    board[all_monster[id].pos[1]][all_monster[id].pos[0]].type = ' ';
    board[all_monster[id].pos[1]][all_monster[id].pos[0]].id = DISCONNECT;
    client_sockets[id][1] = DISCONNECT;
}                      


void player_data(int *sock, char_data previous){
    int color[3];
    int rand_pos[2];

    if(occupied_places == dimensions[0]*dimensions[1]){
        int kick = KICK;
        send(sock[0], &kick, sizeof(int), 0);
        pthread_exit(NULL);
    }
    occupied_places = occupied_places + 2;
    

    send(sock[0], &sock[1], sizeof(int), 0);                //sends id
    send(sock[0], dimensions, (sizeof(int) * 2), 0);        //sends board's dimensions
    recv(sock[0], color, (sizeof(int) * 3), 0);             //receives color of player
   
    init_character(&all_pac[sock[1]], PACMAN, sock[1], CONNECT, color[0], color[1], color[2]);
    init_character(&all_monster[sock[1]], MONSTER, sock[1], CONNECT, color[0], color[1], color[2]);
    init_character(&previous, 0, 0, 0, 0, 0, 0);
    for(int i = 0; i < dimensions[1]; i++){
        send(sock[0], board[i], (sizeof(board_struct)*dimensions[0]), 0);
        for(int j = 0; j < dimensions[0]; j++){
            printf("%c", board[i][j].type);
        }
        printf("\n");
    }

    for(int i = 0; i < 2; i++){
        do{
            rand_pos[0] = rand() % dimensions[0];
            rand_pos[1] = rand() % dimensions[1];             //creates random starting positions
        }while(board[rand_pos[1]][rand_pos[0]].type != ' ');
        if(i == 0){
            all_pac[sock[1]].pos[0] = rand_pos[0];
            all_pac[sock[1]].pos[1] = rand_pos[1]; 
            board[all_pac[sock[1]].pos[1]][all_pac[sock[1]].pos[0]].type = 'P';      
            board[all_pac[sock[1]].pos[1]][all_pac[sock[1]].pos[0]].id = sock[1];       
            push_update(all_pac[sock[1]], previous);          
        }
        else{
            all_monster[sock[1]].pos[0] = rand_pos[0];
            all_monster[sock[1]].pos[1] = rand_pos[1];
            board[all_monster[sock[1]].pos[1]][all_monster[sock[1]].pos[0]].type = 'M';             
            board[all_monster[sock[1]].pos[1]][all_monster[sock[1]].pos[0]].id = sock[1];
            push_update(all_monster[sock[1]], previous);            //paints starting positions on server
        }                
    }

    send(sock[0], all_pac, (sizeof(char_data) * MAX_CLIENT), 0);       //sends to the client all other players connected
    send(sock[0], all_monster, (sizeof(char_data) * MAX_CLIENT), 0);
    send_update(all_pac[sock[1]]);
    send_update(all_monster[sock[1]]);  //sends to all other clients the newly connected player
}

int bounce_on_walls(char_data update, char_data character[MAX_CLIENT]){
    int id = update.id;
    int ret = 1;
    if(update.pos[0] < 0){
        if(board[character[id].pos[1]][1].type != 'B')   //only if spot is empty
            character[id].pos[0] = 1;  //bounce on the left border        
    }
    else if(update.pos[0] == dimensions[0]){
        if(board[character[id].pos[1]][dimensions[0] - 2].type != 'B')   //only if spot is empty
            character[id].pos[0] = dimensions[0] - 2;  //bounce on the right border
    }
    else if(update.pos[1] < 0){
        if(board[1][character[id].pos[0]].type != 'B')   //only if spot is empty
            character[id].pos[1] = 1;  //bounce on the top border
    }
    else if(update.pos[1] == dimensions[1]){
        if(board[dimensions[1] - 2][character[id].pos[0]].type != 'B')   //only if spot is empty
            character[id].pos[1] = dimensions[1] - 2;  //bounce on the bottom border
    }
    else{
        ret = 0;
    }
    return(ret);
}

void bounce_on_brick(int id, char_data character[MAX_CLIENT], char_data previous){
    if(board[character[id].pos[1]][character[id].pos[0]].type == 'B'){
        if(previous.pos[0] == character[id].pos[0]){          
            if(previous.pos[1] > character[id].pos[1]){
                if(((character[id].pos[1] + 2) < dimensions[1]) && (board[character[id].pos[1] + 2][character[id].pos[0]].type != 'B')){
                    character[id].pos[1] = character[id].pos[1] + 2;                    
                }
                else{
                    character[id] = previous;
                }                    
            }      
            else{
                if(((character[id].pos[1] - 2) >= 0) && (board[character[id].pos[1] - 2][character[id].pos[0]].type != 'B')){
                    character[id].pos[1] = character[id].pos[1] - 2;
                }
                else{
                    character[id] = previous;
                }
            }
        }      
        else if(previous.pos[1] == character[id].pos[1]){
            if(previous.pos[0] > character[id].pos[0]){
                if(((character[id].pos[0] + 2) < dimensions[0]) && (board[character[id].pos[1]][character[id].pos[0] + 2].type != 'B')){
                    character[id].pos[0] = character[id].pos[0] + 2;
                }
                else{
                    character[id] = previous;
                }
            }
            else{
                if(((character[id].pos[0] - 2) >= 0) && (board[character[id].pos[1]][character[id].pos[0] - 2].type != 'B')){
                    character[id].pos[0] = character[id].pos[0] - 2;
                }
                else{
                    character[id] = previous;
                }
            }
        } 
    }
}

int character_interactions(int id, char_data character[MAX_CLIENT], char_data previous_pac, char_data previous_monster){
    int occupant_id;
    if(character[id].type == PACMAN){
        if(board[character[id].pos[1]][character[id].pos[0]].id == id){    //if it's the monster of the same player
            printf("position occupied by your monster\n");
            character[id] = previous_pac;
            change_positions(&character[id], 'P', &all_monster[id], 'M', id);                      
            return(1);
        }
        else if(board[character[id].pos[1]][character[id].pos[0]].type == 'P' || board[character[id].pos[1]][character[id].pos[0]].type == 'S'){   //if it's another pacman(super or not)
            occupant_id = board[character[id].pos[1]][character[id].pos[0]].id;
            printf("position occupied by pacman of player %d\n", occupant_id);
            character[id] = previous_pac;
            change_positions(&character[id], 'P', &all_pac[occupant_id], 'P', occupant_id);
            return(1);
        }
        else if(board[character[id].pos[1]][character[id].pos[0]].type == 'M'){
            eat(&all_pac[id], 'P', PACMAN);
            return(1);
        }
    }
    else if(character[id].type == MONSTER){
        if(board[character[id].pos[1]][character[id].pos[0]].id == id){    //if it's the pacman of the same player
            printf("position occupied\n");
            character[id] = previous_monster;
            change_positions(&character[id], 'M', &all_pac[id], 'P', id);                    
            return(1);
        }
        else if(board[character[id].pos[1]][character[id].pos[0]].type == 'M'){      //if it's a monster from another player
            occupant_id = board[character[id].pos[1]][character[id].pos[0]].id;
            printf("position occupied by monster of player %d\n", occupant_id);
            character[id] = previous_monster;
            change_positions(&character[id], 'M', &all_monster[occupant_id], 'M', occupant_id);
            return(1);
        }
        else if(board[character[id].pos[1]][character[id].pos[0]].type == 'P'){
            occupant_id = board[character[id].pos[1]][character[id].pos[0]].id;
            eat(&all_pac[occupant_id], 'P', MONSTER);
            return(1);
        }
    }
    all_pac[id].state = CONNECT;
    all_monster[id].state = CONNECT;
    return(0);
}

void change_positions(char_data *pos_1, char type_1, char_data *pos_2, char type_2, int occupant_id){
    int aux[2];        
    char_data previous = *pos_1;  //this is just to initialize this value; it won't be used cause the state is CHANGE so the SDL event won't access this variable
    aux[0] = pos_1->pos[0];
    aux[1] = pos_1->pos[1];
    pos_1->pos[0] = pos_2->pos[0];
    pos_1->pos[1] = pos_2->pos[1];
    pos_2->pos[0] = aux[0];
    pos_2->pos[1] = aux[1];
    board[pos_2->pos[1]][pos_2->pos[0]].type = type_2;
    board[pos_2->pos[1]][pos_2->pos[0]].id = occupant_id;  
    pos_2->state = CHANGE;  
    pos_1->state = CHANGE;
    push_update(*pos_2, previous);
    send_update(*pos_2);
}

void eat(char_data *eaten, char eaten_type, int moving_type){
    int rand_pos0;
    int rand_pos1;
    char_data previous = *eaten;
    do{
        rand_pos0 = rand()%dimensions[0];
        rand_pos1 = rand()%dimensions[1];
    }while(board[rand_pos1][rand_pos0].type != ' ');
    printf("rand %d %d\n", rand_pos0, rand_pos1);
    eaten->pos[0] = rand_pos0;
    eaten->pos[1] = rand_pos1;    
    if(eaten->type != moving_type){   //if who is eaten is not the one moving
        printf("the one eaten was not moving\n");
        board[eaten->pos[1]][eaten->pos[0]].type = eaten_type;
        board[eaten->pos[1]][eaten->pos[0]].id = eaten->id;
        eaten->state = CHANGE;
        push_update(*eaten, previous);
        send_update(*eaten);
    }
}