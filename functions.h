#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define PORT 55555
#define MAX_CLIENT 10
#define PACMAN 0
#define MONSTER 1
#define FRUIT 2
#define POWER_PACMAN 3
#define EAT_1 4
#define EAT_2 5
#define NOT_CONNECT -1
#define CONNECT -2
#define DISCONNECT -3
#define ENDGAME -4
#define KICK -5
#define CHANGE -6
#define JUST_UPDATE_VAR -7
#define ERASE_FRUIT -8
#define IDLE -9
#define SPEED 000000000   //minimum nanoseconds allowed between moves
#define INACTIVITY 10
#define WRITE 1
#define READ 0

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
void push_update(char_data update, char_data previous, pthread_mutex_t *mux_sdl);
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
int character_interactions(int id, char_data character[MAX_CLIENT], char_data previous);
//changes the position between two characters
void change_positions(char_data *pos_1, char type_1, char_data *pos_2, char type_2, int occupant_id);
// creates random postition for pacman after being eaten
void eat(char_data *eaten, char eaten_type, int moving_type);
//handles the speed limit on the characters
int over_speed(struct timespec time_of_play, struct timespec *char_play);
//implements inactivity jump
//thread for pacman and monster movements
void *pac_thread(void *);
void *monster_thread(void *);
//all the functions each move has to go trough
void new_move(char_data character[MAX_CLIENT], char_data update, struct timespec time_of_new_play, struct timespec *time_of_char_play);
//creates a random starting postition that's not occupied by another character
void create_rand_position(int *rand_pos);
//thread function that adds and clears the fruits on the board
void *fruits_thread(void *arg);
//clears the two fruits associated with the client disconecting
void clear_fruits(char_data *update);
//creates the new random postition for the fruit after being eaten
void eat_fruit(int id);
//gets a char with the type of character
char get_char_type(char_data character);
//functions that implement inactivity jump
void inactivity_jump(char_data *character);
void *inactivity_timer(void *arg);
//if array of max clients id reaches max, it will return an id of a client that disconnected
int get_id_if_full();



/*___________________________________Player Specific Funtions_______________________________________________*/

//connects to the server
void connect_server(char *argv[], int *sock_fd);
//receives the updated movement any player and pushes it to the SDL queue
void *update_thread(void *arg);
//recvs and sends to server initial data needed
void server_data(int sock_fd, char *argv[]);
//paints the bricks and the players already playing
void initial_paint();