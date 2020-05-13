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
    int local_id;
    int dimensions[2];
    pthread_t thread_id;
    

    connect_server(argv, &sock_fd);
    recv(sock_fd, &local_id, sizeof(int), 0);
    recv(sock_fd, dimensions, (sizeof(int) * 2), 0);

    printf("id %d\t %d %d\n", local_id, dimensions[0], dimensions[1]);

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