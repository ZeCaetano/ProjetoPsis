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



int main(int argc, char *argv[]){
    int sock_fd;
    int local_id = 3;
    int dimensions[2];
    int done = 0;
    char_data local_pac;
    char_data local_monster;
    SDL_Event event;
    pthread_t thread_id;
    
    if(argc != 6){
        printf("Insert - ./player address port r g b\n");
        exit(1);
    }

    connect_server(argv, &sock_fd);
    recv(sock_fd, &local_id, sizeof(int), 0);
    recv(sock_fd, dimensions, (sizeof(int) * 2), 0);

    init_character(&local_pac, PACMAN, local_id, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
    init_character(&local_monster, MONSTER, local_id, atoi(argv[3]), atoi(argv[4]), atoi(argv[5]));
    printf("local id %d\n", local_pac.id);
    send(sock_fd, local_pac.color, (sizeof(int) * 3), 0); //sends the color to server

    recv(sock_fd, local_pac.pos, (sizeof(int) * 2), 0);
    recv(sock_fd, local_monster.pos, (sizeof(int) * 2), 0);
    printf("random positions to send: %d %d\n", local_pac.pos[0], local_pac.pos[1]);
    printf("random positions to send: %d %d\n", local_monster.pos[0], local_monster.pos[1]);
    


    create_board_window(dimensions[0], dimensions[1]);

    while(!done){
        if(SDL_WaitEvent(&event)){
            if(event.type == SDL_QUIT){
                done = SDL_TRUE;
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