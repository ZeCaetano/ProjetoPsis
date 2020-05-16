#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define PORT 55555
#define MAX_CLIENT 10
#define PACMAN 0
#define MONSTER 1
#define NOT_CONNECT -1
#define CONNECT -123
#define DISCONNECT -111
#define ENDGAME -333
#define KICK -555
#define CHANGE -43

typedef struct char_data{   //character data
    int pos[2];             //position
    int color[3];           //color, rgb
    int type;               //pacman or monster
    int id;                 //id given by server
    int state;              //not connected, connected, disconected
} char_data;

typedef struct board_struct{
    char type;
    int id;
} board_struct;

/*_______________________________________General Functions______________________________________________*/

//malloc with the if condition to check if it succeeded
void *checked_malloc(size_t size);
//initializes the struct for client data
void init_character(char_data *character, int type, int id, int state, int r, int g, int b);
//pushes the updated move to the sdl queue
void push_update(char_data update, char_data previous);
//cleares the old position and paints in the new position
void paint_update(char_data *data, char_data *previous, char_data all_pac[MAX_CLIENT], char_data all_monster[MAX_CLIENT]);


/*__________________________________Server Specific Functions____________________________________________*/

//function that opens and reads the board.txt witht the information about the dimensions and bricks
void read_file();
//inializes sockets and connects to the client  
void *connect_client(void *arg);  
//receives updates from client, analyses it and pushes it to the SDL queue
void *client(void *arg);
//checks if the new position the player wants to go is different than the position the character was
//and if the new position is one of the four adjacent possible positions
int valid_movement(char_data update, char_data character[MAX_CLIENT]);
//sends the new update to every connectec player
void send_update(char_data update);
//sets variables to a disconnect state and sends this information to all other players
void disconnect_player(int id);
//sends and receives initial player data needed
void player_data(int *sock, char_data previous);
//implements the bounce on walls feature
int bounce_on_walls(char_data update, char_data character[MAX_CLIENT]);
//implements the bounce on bricks feature
void bounce_on_brick(int id, char_data character[MAX_CLIENT], char_data previous);
//implements all the interactions between characters
int character_interactions(int id, char_data character[MAX_CLIENT], char_data previous_pac, char_data previous_monster);
//changes the position between two characters
void change_positions(char_data *pos_1, char type_1, char_data *pos_2, char type_2);




/*___________________________________Player Specific Funtions_______________________________________________*/

//connects to the server
void connect_server(char *argv[], int *sock_fd);
//receives the updated movement any player and pushes it to the SDL queue
void *update_thread(void *arg);
//recvs and sends to server initial data needed
void server_data(int sock_fd, char *argv[]);
//paints the bricks and the players already playing
void initial_paint();