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

Uint32 Event_Update;
Uint32 Event_bot;


char_data all_pac[MAX_CLIENT];
char_data all_monster[MAX_CLIENT];
char_data local_pac;
char_data local_monster;
int local_id = 0;
int dimensions[2];
board_struct **board;
pthread_mutex_t mux_sdl;


int main(int argc, char *argv[]){
    int sock_fd;    
   
    int done = 0;

    SDL_Event event;
    pthread_t thread_id;
    
    if(argc != 6){
        printf("Insert - ./player address port r g b\n");
        exit(1);
    }

    connect_server(argv, &sock_fd);
    server_data(sock_fd, argv);
    local_pac = all_pac[local_id];
    local_monster = all_monster[local_id];

    pthread_mutex_init(&mux_sdl, NULL);
        
    pthread_create(&thread_id, NULL, update_thread, (void *)&sock_fd);

    create_board_window(dimensions[0], dimensions[1]);
    initial_paint();

    while(!done){
     /*   local_pac.pos[0] = rand()%dimensions[0];
        local_pac.pos[1] = rand()%dimensions[1];
        local_monster.pos[0] = rand()%dimensions[0];
        local_monster.pos[1] = rand()%dimensions[1];
        send(sock_fd, &local_pac, sizeof(char_data), 0);
        send(sock_fd, &local_monster, sizeof(char_data), 0);*/
        if(SDL_WaitEvent(&event)){
            if(event.type == SDL_QUIT){
                done = SDL_TRUE;
            }
            if(event.type == Event_Update){
                char_data *data = event.user.data1;
                char_data *previous = event.user.data2;
                paint_update(data, previous, all_pac, all_monster);
                if(data->state == ENDGAME){
                    free(previous);
                    free(data);
                    printf("Kicked by admin\n");
                    done = SDL_TRUE;
                }
                free(previous);
                free(data);
            }
            if(event.type == SDL_MOUSEMOTION){
                get_board_place(event.motion.x, event.motion.y, &local_pac.pos[0], &local_pac.pos[1]);                
                send(sock_fd, &local_pac, sizeof(char_data), 0);
            }
            if(event.type == SDL_KEYDOWN){
                if(event.key.keysym.sym == SDLK_LEFT){
                    local_monster.pos[0] = local_monster.pos[0] - 1;                    
                }
                else if(event.key.keysym.sym == SDLK_RIGHT){
                    local_monster.pos[0] = local_monster.pos[0] + 1;                    
                }
                else if(event.key.keysym.sym == SDLK_UP){
                    local_monster.pos[1] = local_monster.pos[1] - 1;
                }
                else if(event.key.keysym.sym == SDLK_DOWN){
                    local_monster.pos[1] = local_monster.pos[1] + 1;                    
                }
                send(sock_fd, &local_monster, sizeof(char_data), 0);
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


void connect_server(char *argv[], int *sock_fd){
    int err = 0;
    char *addr;
    struct sockaddr_in server_addr;

    addr = argv[1];
    int port = atoi(argv[2]);
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_aton(addr, &server_addr.sin_addr);

    socklen_t addr_len = sizeof(server_addr);

    *sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(*sock_fd == -1){
        perror("socket");
        exit(-1);
    }

    err = connect(*sock_fd, (struct sockaddr *)&server_addr, addr_len);
    if(err == -1){
        printf("Error connecting\n");
        exit(-1);
    }    
    printf("connected to server\n");    
}

void *update_thread(void *arg){
    int *sock_fd = arg;    
    char_data update;    
    char_data previous;
    
    while(1){
        if(recv(*sock_fd, &update, sizeof(char_data), 0) == 0){
            //server closed
        }
        if(update.state == DISCONNECT){  //a client disconected
            all_pac[update.id] = update;
            push_update(update, previous, &mux_sdl);
        }
        else if(update.state == ENDGAME){
            push_update(update, previous, &mux_sdl);
        }
        else if(update.state == JUST_UPDATE_VAR){
            local_monster = update;
        }  
        else if(update.type == PACMAN){
            previous = all_pac[update.id];
            all_pac[update.id] = update;
            local_pac = all_pac[local_id];
            push_update(all_pac[update.id], previous, &mux_sdl);
        }
        else if(update.type ==  MONSTER){
            previous = all_monster[update.id];
            all_monster[update.id] = update;
            local_monster = all_monster[local_id];
            push_update(all_monster[update.id], previous, &mux_sdl);
        }   
        printf("x%d y%d \tid %d type %d\n", update.pos[0], update.pos[1], update.id, update.type);             
    }
}   

void server_data(int sock_fd, char *argv[]){
    int color[3];
    color[0] = atoi(argv[3]);
    color[1] = atoi(argv[4]);
    color[2] = atoi(argv[5]);
    recv(sock_fd, &local_id, sizeof(int), 0);
    if(local_id == KICK){
        printf("You were kicked, board is full");
        exit(0);
    }
    recv(sock_fd, dimensions, (sizeof(int) * 2), 0);
    send(sock_fd, color, (sizeof(int) * 3), 0);                  //sends the color to server
    printf("dimensions: %d %d\n", dimensions[0], dimensions[1]);
    board = checked_malloc(sizeof(board_struct*) * dimensions[1]);             //rows
    for(int i = 0; i < dimensions[1]; i++){
        board[i] = checked_malloc(sizeof(board_struct) * dimensions[0]);      //columns
        recv(sock_fd, board[i], (sizeof(board_struct)*dimensions[0]), 0);
    }    

    printf("local id %d\n", local_id);

    recv(sock_fd, all_pac, (sizeof(char_data) * MAX_CLIENT), 0);
    recv(sock_fd, all_monster, (sizeof(char_data) * MAX_CLIENT), 0);
}

void initial_paint(){
    for(int i = 0; i < dimensions[1]; i++){
        for(int j = 0; j < dimensions[0]; j++){
            if(board[i][j].type == 'B'){
                paint_brick(j, i);
            }
        }
    }
    for(int i = 0; i < MAX_CLIENT; i++){
        if((all_pac[i].state != DISCONNECT) && (all_pac[i].state != NOT_CONNECT)){
            paint_pacman(all_pac[i].pos[0],all_pac[i].pos[1], all_pac[i].color[0], all_pac[i].color[1], all_pac[i].color[2]);
            paint_monster(all_monster[i].pos[0],all_monster[i].pos[1], all_monster[i].color[0], all_monster[i].color[1], all_monster[i].color[2]);
        }
    }
}