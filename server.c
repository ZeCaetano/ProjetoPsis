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
#include <signal.h>
#include "time.h"

#include "UI_library.h"
#include "functions.h"


Uint32 Event_Update;


int dimensions[2];   //dimensions of the board
board_struct **board;        //matrix with positions occupied by pacmans, monsters, fruits, bricks
int client_sockets[MAX_CLIENT][2];     //socket for every client; [0] for socket, [1] for client it corresponds
char_data all_pac[MAX_CLIENT];
char_data all_monster[MAX_CLIENT];
//char_data update;
int occupied_places;
pthread_mutex_t mux_interactions;
pthread_mutex_t mux_sdl;
struct timespec time_of_pac_play[MAX_CLIENT];
struct timespec time_of_monster_play[MAX_CLIENT];
int new_pac_move_pipe[MAX_CLIENT][2];
int new_monster_move_pipe[MAX_CLIENT][2];
int current_players;
int n_players;
int inactive;
int fruit_pipe[2];
int fruits_pos[(MAX_CLIENT-1)*2][2];

int main(){
    pthread_t connect_thread;
    pthread_t fruits_thread_id;
    pthread_t inactivity_thread_id;
    srand(time(NULL));
    int done = 0;
    SDL_Event event;
    occupied_places = 0; 
    current_players = 0;
    n_players = 0;

    for(int i = 0; i < MAX_CLIENT; i++){
        client_sockets[i][1] = DISCONNECT;
    }    
    if(pipe(fruit_pipe) != 0){
        exit(-1);
    }

    for(int i = 0; i < MAX_CLIENT; i++){
        init_character(&all_pac[i], PACMAN, i, NOT_CONNECT, 0, 0, 0);
        init_character(&all_monster[i], MONSTER, i, NOT_CONNECT, 0, 0, 0);
    }
    read_file();
    pthread_mutex_init(&mux_interactions, NULL);
    pthread_mutex_init(&mux_sdl, NULL);
    pthread_create(&connect_thread, NULL, connect_client, NULL);
    pthread_create(&fruits_thread_id, NULL, fruits_thread, NULL);
    pthread_create(&inactivity_thread_id, NULL, inactivity_timer, NULL);


    alarm(SCORE_TIME);
    signal(SIGALRM, send_scoreboard);

   
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
    pthread_t client_thread_id;
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
        pthread_create(&client_thread_id, NULL, client, (void *)client_sockets[client_id]);
        if(client_id == MAX_CLIENT){
            client_id = get_id_if_full();
        }
        else{
            client_id ++;
        }
    }
    pthread_exit(NULL);
}


void *client(void *arg){
    int *sock = (int*)arg;
    int ret;
    pthread_t pac_thread_id;
    pthread_t monster_thread_id;
    char_data update;
    char_data previous;
    struct timespec time_of_new_play;

    printf("New client thread created\n");

    if(pipe(new_pac_move_pipe[sock[1]]) != 0){
        exit(-1);
    }
    if(pipe(new_monster_move_pipe[sock[1]]) != 0){
        exit(-1);
    }

    pthread_create(&pac_thread_id, NULL, pac_thread, NULL);
    write(new_pac_move_pipe[sock[1]][WRITE], &sock[1], sizeof(int));
    pthread_create(&monster_thread_id, NULL, monster_thread, NULL);
    write(new_monster_move_pipe[sock[1]][WRITE], &sock[1], sizeof(int));

    player_data(sock, previous);
        
    while(1){    
        ret = recv(sock[0], &update, sizeof(char_data), 0);
        clock_gettime(CLOCK_MONOTONIC, &time_of_new_play);              //reset counter every new move
        if(ret == 0){              
            disconnect_player(update.id);
            printf("Client disconnected \n");
            update.state = DISCONNECT;
            push_update(update, previous, &mux_sdl);
            send_update(all_pac[update.id]);
            break;
        }
        if(update.type == PACMAN || update.type >= POWER_PACMAN){
            write(new_pac_move_pipe[sock[1]][WRITE], &update, sizeof(char_data));   
            write(new_pac_move_pipe[sock[1]][WRITE], &time_of_new_play, sizeof(struct timespec));                       
        }
        else if(update.type == MONSTER){
            write(new_monster_move_pipe[sock[1]][WRITE], &update, sizeof(char_data));   
            write(new_monster_move_pipe[sock[1]][WRITE], &time_of_new_play, sizeof(struct timespec));
        }               
        
        
   
        /*for(int i = 0; i < dimensions[1]; i++){
            for(int j = 0; j < dimensions[0]; j++){
                printf("%c.%d", board[i][j].type, board[i][j].id);
            }
            printf("\n");
        }*/         
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
        || (update.pos[1] == character[update.id].pos[1] - 1 && update.pos[0] == character[update.id].pos[0])){   //if it's any of the adjacent places
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
    int fruit = -1;
    all_pac[id].id = id;
    all_pac[id].state = DISCONNECT;
    board[all_pac[id].pos[1]][all_pac[id].pos[0]].type = ' ';
    board[all_pac[id].pos[1]][all_pac[id].pos[0]].id = DISCONNECT;
    board[all_monster[id].pos[1]][all_monster[id].pos[0]].type = ' ';
    board[all_monster[id].pos[1]][all_monster[id].pos[0]].id = DISCONNECT;
    current_players--;
    write(fruit_pipe[WRITE], &fruit, sizeof(int));
    client_sockets[id][1] = DISCONNECT;
    if(current_players == 1){
        printf("only one player playing\n");
        for(int i = 0; i < MAX_CLIENT; i++){
            if(all_pac[i].state ==  CONNECT){
                all_pac[i].eaten_things = 0;
            }
        }
    }
}                      


void player_data(int *sock, char_data previous){
    int color[3];
    int rand_pos[2];
    int fruit = -1;

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
     /*   for(int j = 0; j < dimensions[0]; j++){
           printf("%c", board[i][j].type);
        }
        printf("\n");*/
    }

    for(int i = 0; i < 2; i++){
        create_rand_position(rand_pos);
        if(i == 0){
            all_pac[sock[1]].pos[0] = rand_pos[0];
            all_pac[sock[1]].pos[1] = rand_pos[1]; 
            board[all_pac[sock[1]].pos[1]][all_pac[sock[1]].pos[0]].type = 'P';      
            board[all_pac[sock[1]].pos[1]][all_pac[sock[1]].pos[0]].id = sock[1];       
            push_update(all_pac[sock[1]], previous, &mux_sdl);          
        }
        else{
            all_monster[sock[1]].pos[0] = rand_pos[0];
            all_monster[sock[1]].pos[1] = rand_pos[1];
            board[all_monster[sock[1]].pos[1]][all_monster[sock[1]].pos[0]].type = 'M';             
            board[all_monster[sock[1]].pos[1]][all_monster[sock[1]].pos[0]].id = sock[1];
            push_update(all_monster[sock[1]], previous, &mux_sdl);            //paints starting positions on server
        }                
    }

    send(sock[0], all_pac, (sizeof(char_data) * MAX_CLIENT), 0);       //sends to the client all other players connected
    send(sock[0], all_monster, (sizeof(char_data) * MAX_CLIENT), 0);
    send_update(all_pac[sock[1]]);
    send_update(all_monster[sock[1]]);  //sends to all clients the newly connected player
    write(fruit_pipe[WRITE], &fruit, sizeof(int));
    current_players ++;
    n_players++;

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

int character_interactions(int id, char_data character[MAX_CLIENT], char_data previous){
    int occupant_id;
    if(character[id].type == PACMAN || character[id].type >= POWER_PACMAN){
        if(board[character[id].pos[1]][character[id].pos[0]].id == id && board[character[id].pos[1]][character[id].pos[0]].type != 'F'){    //if it's the monster of the same player
          //  printf("position occupied by your monster\n");
            character[id] = previous;
            change_positions(&character[id], get_char_type(character[id]), &all_monster[id], 'M', id);                      
            return(1);
        }
        else if(board[character[id].pos[1]][character[id].pos[0]].type == 'P' || board[character[id].pos[1]][character[id].pos[0]].type == 'S'){   //if it's another pacman(super or not)
            occupant_id = board[character[id].pos[1]][character[id].pos[0]].id;
          //  printf("position occupied by pacman of player %d\n", occupant_id);
            character[id] = previous;
            change_positions(&character[id], 'P', &all_pac[occupant_id], 'P', occupant_id);
            return(1);
        }
        else if(board[character[id].pos[1]][character[id].pos[0]].type == 'M'){
            occupant_id = board[character[id].pos[1]][character[id].pos[0]].id;
            if(character[id].type == PACMAN){
                all_pac[occupant_id].eaten_things++;
                eat(&all_pac[id], 'P', PACMAN);
                return(1);                
            }
            else if(character[id].type >= POWER_PACMAN){
                eat(&all_monster[occupant_id], 'M', POWER_PACMAN);
                character[id].type++;
                all_pac[id].eaten_things++;
                if(character[id].type == EAT_2){
                    character[id].type = PACMAN;
                }
                return(1);
            }
        }
        else if(board[character[id].pos[1]][character[id].pos[0]].type == 'F'){
            occupant_id = board[character[id].pos[1]][character[id].pos[0]].id;
            character[id].type = POWER_PACMAN;
            all_pac[id].eaten_things++;
            write(fruit_pipe[WRITE], &occupant_id, sizeof(int));
            return(2);
        }
    
    }
    else if(character[id].type == MONSTER){
        if(board[character[id].pos[1]][character[id].pos[0]].id == id && board[character[id].pos[1]][character[id].pos[0]].type != 'F'){    //if it's the pacman of the same player
           // printf("position occupied\n");
            character[id] = previous;
            change_positions(&character[id], 'M', &all_pac[id], 'P', id);                    
            return(1);
        }
        else if(board[character[id].pos[1]][character[id].pos[0]].type == 'M'){      //if it's a monster from another player
            occupant_id = board[character[id].pos[1]][character[id].pos[0]].id;
            //printf("position occupied by monster of player %d\n", occupant_id);
            character[id] = previous;
            change_positions(&character[id], 'M', &all_monster[occupant_id], 'M', occupant_id);
            return(1);
        }
        else if(board[character[id].pos[1]][character[id].pos[0]].type == 'P'){
            occupant_id = board[character[id].pos[1]][character[id].pos[0]].id;
            all_pac[id].eaten_things++;
            eat(&all_pac[occupant_id], 'P', MONSTER);
            return(1);
        }
        else if(board[character[id].pos[1]][character[id].pos[0]].type == 'F'){
            occupant_id = board[character[id].pos[1]][character[id].pos[0]].id;
            all_pac[id].eaten_things++;
            write(fruit_pipe[WRITE], &occupant_id, sizeof(int));
            return(1);
        }
        else if(board[character[id].pos[1]][character[id].pos[0]].type == 'S'){
            occupant_id = board[character[id].pos[1]][character[id].pos[0]].id;
            all_pac[occupant_id].eaten_things++;
            eat(&all_monster[id], 'M', MONSTER);
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
    push_update(*pos_2, previous, &mux_sdl);
    send_update(*pos_2);
}

void eat(char_data *eaten, char eaten_type, int moving_type){
    int rand_pos[2];
    char_data previous = *eaten;
    create_rand_position(rand_pos);
  //  printf("rand %d %d\n", rand_pos[0], rand_pos[1]);
    eaten->pos[0] = rand_pos[0];
    eaten->pos[1] = rand_pos[1];    
    if(eaten->type != moving_type){   //if who is eaten is not the one moving
        board[eaten->pos[1]][eaten->pos[0]].type = eaten_type;
        board[eaten->pos[1]][eaten->pos[0]].id = eaten->id;
        eaten->state = CHANGE;
        push_update(*eaten, previous, &mux_sdl);
        send_update(*eaten);
    }
}

int over_speed(struct timespec time_of_play, struct timespec *char_play){
    long int delta = 0;
   // printf("seconds time diference %ld\n", (time_of_play.tv_sec - char_play->tv_sec));
    delta = time_of_play.tv_nsec - char_play->tv_nsec;
    
    if(((time_of_play.tv_sec - char_play->tv_sec) == 0)){      //if it's the same second, just need to subtract the nanoseconds
     //   printf("same second\n");
        //printf("nanoseconds time difference%ld\n", delta);
        if(delta < SPEED){
       //     printf("TOO FAST\n");
            return(1);
        }
        else{
            *char_play = time_of_play;
            return(0);
        }
    }
    else if((time_of_play.tv_sec - char_play->tv_sec) == 1){    //if it's the next second, nanoseconds might be lower,
      //  printf("next second\n");                                    //so the sub will be negative, and it's needed to convert to actual nanoseconds passed    
        if(delta < 0){
            delta = 1000000000 + delta;
        }
        //printf("nanoseconds time difference%ld\n", delta);
        if(delta < SPEED){
        //    printf("TOO FAST\n");
            return(1);
        }
        else {
            *char_play = time_of_play;
            return(0);
        }            
    }
    else{
       // printf("a few seconds after\n");
       // printf("nanoseconds time difference %ld\n", delta);
        *char_play = time_of_play;
        return(0);
    }       
}

void new_move(char_data character[MAX_CLIENT], char_data update, struct timespec time_of_new_play, struct timespec *time_of_char_play){
    char type;
    char_data previous = character[update.id];                

    if(valid_movement(update, character)){
        if(over_speed(time_of_new_play, time_of_char_play) == 0){
            if(bounce_on_walls(update, character) == 0){          //if it bounces on a wall it won't bounce on a brick
                character[update.id] = update;                    //same goes for bounce on brick
                bounce_on_brick(update.id, character, previous);                    
            }                         
            board[previous.pos[1]][previous.pos[0]].type = ' ';
            board[previous.pos[1]][previous.pos[0]].id = NOT_CONNECT;
            pthread_mutex_lock(&mux_interactions);   
            character_interactions(update.id, character, previous);
            pthread_mutex_unlock(&mux_interactions);       
            type = get_char_type(character[update.id]);
            board[character[update.id].pos[1]][character[update.id].pos[0]].type = type;
            board[character[update.id].pos[1]][character[update.id].pos[0]].id = update.id;                  
            push_update(character[update.id], previous, &mux_sdl);
            send_update(character[update.id]);                
        }
        else{
            character[update.id].state = JUST_UPDATE_VAR;
            send_update(character[update.id]);
        }
    }
}

void create_rand_position(int *rand_pos){
    do{
        rand_pos[0] = rand() % dimensions[0];
        rand_pos[1] = rand() % dimensions[1];             //creates random starting positions
    }while(board[rand_pos[1]][rand_pos[0]].type != ' ');
}

void *fruits_thread(void *arg){
    int n_players_aux = 0;
    int rand_pos[2];
    int id;
    int n_fruits = 0;
    int players_disconnected = 0;
    char_data fruit; 
    init_character(&fruit, FRUIT, 0, 0, 0, 0, 0);
    while(1){
        if(current_players > n_players_aux){      //new player connected
            if(current_players> 1){   
                for(int i = 0; i < 2; i++){             //(current_players- 1)*2 means that for every new player, there's two more fruits
                    if(occupied_places < dimensions[0]*dimensions[1]){
                        create_rand_position(rand_pos);
                        board[rand_pos[1]][rand_pos[0]].type = 'F';
                        board[rand_pos[1]][rand_pos[0]].id = n_fruits;
                        fruit.pos[0] = rand_pos[0];
                        fruit.pos[1] = rand_pos[1];
                        fruit.type = FRUIT;
                        fruit.state = CONNECT;
                        fruits_pos[n_fruits][0] = fruit.pos[0];
                        fruits_pos[n_fruits][1] = fruit.pos[1];
                        push_update(fruit, fruit, &mux_sdl);                    
                        send_update(fruit);
                        n_fruits++; 
                        if(n_fruits == (MAX_CLIENT - 1)*2){
                            n_fruits = 0;
                            players_disconnected = 0;        //if max number of fruits is reached reset the ids of fruits array
                        }
                        occupied_places++;
                    }                    
                }    
            }
            n_players_aux = current_players;
        }
        if(current_players < n_players_aux){    //player disconnected
            for(int i = players_disconnected * 2; i <= players_disconnected * 2 + 1; i++){
                fruit.pos[0] = fruits_pos[i][0];
                fruit.pos[1] = fruits_pos[i][1];
                fruit.type = FRUIT;
                fruit.state = ERASE_FRUIT;
                push_update(fruit, fruit, &mux_sdl);
                send_update(fruit);
            }
            players_disconnected++;
            n_players_aux = current_players;
        }
        read(fruit_pipe[READ], &id, sizeof(int));
        if(id != -1){
            sleep(2);
            eat_fruit(id);
        }
        usleep(1000);
    }
}

void *pac_thread(void *arg){
    int id;
    int pipe_id;
    char_data update;
    struct timespec time_of_new_play;
    if(n_players == MAX_CLIENT){
        pipe_id = get_id_if_full();
    }
    else{
        pipe_id = n_players;
    }
    read(new_pac_move_pipe[pipe_id][READ], &id, sizeof(int));
    
    clock_gettime(CLOCK_MONOTONIC, &time_of_pac_play[id]);   //to get the time the client connects
    while(1){
        read(new_pac_move_pipe[id][READ], &update, sizeof(char_data));
        read(new_pac_move_pipe[id][READ], &time_of_new_play, sizeof(struct timespec));    
        new_move(all_pac, update, time_of_new_play, &time_of_pac_play[id]);
    }
}

void *monster_thread(void *arg){
    int id;
    char_data update;
    int pipe_id;
    struct timespec time_of_new_play;
    if(n_players == MAX_CLIENT){
        pipe_id = get_id_if_full();
    }
    else{
        pipe_id = n_players;
    }
    read(new_monster_move_pipe[pipe_id][READ], &id, sizeof(int));
    clock_gettime(CLOCK_MONOTONIC, &time_of_monster_play[id]);
    while(1){
        read(new_monster_move_pipe[id][READ], &update, sizeof(char_data)); 
        read(new_monster_move_pipe[id][READ], &time_of_new_play, sizeof(struct timespec));       
        new_move(all_monster, update, time_of_new_play, &time_of_monster_play[id]);
    }
}


void eat_fruit(int id){
    char_data fruit;
    create_rand_position(fruit.pos);
    fruit.type = FRUIT;
    fruit.state = CONNECT;
    board[fruit.pos[1]][fruit.pos[0]].type = 'F';
    board[fruit.pos[1]][fruit.pos[0]].id = id;
    fruits_pos[id][0] = fruit.pos[0];
    fruits_pos[id][1] = fruit.pos[1];    
    push_update(fruit, fruit, &mux_sdl);
    send_update(fruit);
}

char get_char_type(char_data character){
    if(character.type == PACMAN){
        return('P');
    }
    else if(character.type >= POWER_PACMAN){
        return('S');
    }
    else if(character.type == MONSTER){
        return('M');
    }
    else if(character.type == FRUIT){
        return('F');
    }
    return(' ');
}

void inactivity_jump(char_data *character){
    int rand_pos[2];
    char type;
    char_data previous = *character;
    //printf("previous %d %d %d %d\n", previous.pos[0], previous.pos[1], previous.id, previous.type);
    printf("too long inactive\n");
    create_rand_position(rand_pos);
    character->pos[0] = rand_pos[0];
    character->pos[1] = rand_pos[1];
    board[previous.pos[1]][previous.pos[0]].type = ' ';
    board[previous.pos[1]][previous.pos[0]].id = NOT_CONNECT;
    type = get_char_type(*character);
    board[character->pos[1]][character->pos[0]].type = type;
    board[character->pos[1]][character->pos[0]].id = character->id; 
    character->state = CONNECT;          
    push_update(*character, previous, &mux_sdl);
    send_update(*character);
}

void *inactivity_timer(void *arg){
    struct timespec current_time;
    while(1){
        if(n_players > 0){
            clock_gettime(CLOCK_MONOTONIC, &current_time);
            for(int i = 0; i < MAX_CLIENT; i++){
                if(all_pac[i].state != NOT_CONNECT && all_pac[i].state != DISCONNECT){
                    //printf("current%ld\t monster(%d) play %ld\n", current_time.tv_sec, i, time_of_monster_play[i].tv_sec);
                    if(current_time.tv_sec - time_of_pac_play[i].tv_sec == INACTIVITY){
                        inactivity_jump(&all_pac[i]);
                        time_of_pac_play[i] = current_time;
                    }
                    if(current_time.tv_sec - time_of_monster_play[i].tv_sec == INACTIVITY){
                        inactivity_jump(&all_monster[i]);
                        time_of_monster_play[i] = current_time;
                    }
                }
            }
        }
        usleep(100000);
    }
}


int get_id_if_full(){
    for(int i; i < MAX_CLIENT; i++){
        if(all_pac[i].state == DISCONNECT){
            return i;
        }
    }
    return(0);
}

void send_scoreboard(int sign){
    int previous_state = all_pac[0].state;
    pthread_mutex_lock(&mux_interactions);
    all_pac[0].state = SCOREBOARD;
    send_update(all_pac[0]);
    all_pac[0].state = previous_state;
    for(int i = 0; i < MAX_CLIENT; i++){
        send_update(all_pac[i]);
    }
    pthread_mutex_unlock(&mux_interactions);
    alarm(SCORE_TIME);
}