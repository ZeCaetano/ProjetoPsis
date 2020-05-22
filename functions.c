//generic functions

#include "functions.h"
#include "UI_library.h"
#include <pthread.h>
#include "time.h"


Uint32 Event_Update;

void *checked_malloc(size_t size){
    void *ptr;
    ptr = malloc (size);
    if(ptr == NULL){
        printf("Malloc error\n");
        exit(0);
    }
    return ptr;

}


void init_character(char_data *character, int type, int id, int state, int r, int g, int b){
    character->pos[0] = 0;
    character->pos[1] = 0;
    character->color[0] = r;
    character->color[1] = g;
    character->color[2] = b;
    character->id = id;
    character->type = type;    
    character->state = state;
}

void push_update(char_data update, char_data previous, pthread_mutex_t *mux_sdl){
    SDL_Event new_event;
    char_data *event_data[2];

    event_data[0] = malloc(sizeof(char_data));
    event_data[0]->pos[0] = update.pos[0];
    event_data[0]->pos[1] = update.pos[1];
    event_data[0]->color[0] = update.color[0];
    event_data[0]->color[1] = update.color[1];
    event_data[0]->color[2] = update.color[2];
    event_data[0]->type = update.type;    
    event_data[0]->id = update.id;
    event_data[0]->state = update.state;
	event_data[1] = malloc(sizeof(char_data));
	event_data[1]->pos[0] = previous.pos[0];
	event_data[1]->pos[1] = previous.pos[1];
    event_data[1]->state = previous.state;
    pthread_mutex_lock(mux_sdl);                  //to make sure data 1 doesn't get overwritten by another thread
    SDL_zero(new_event);
    new_event.type = Event_Update;
    new_event.user.data1 = event_data[0];
    new_event.user.data2 = event_data[1];
    SDL_PushEvent(&new_event);
    pthread_mutex_unlock(mux_sdl);
}

void paint_update(char_data *data, char_data *previous, char_data all_pac[MAX_CLIENT], char_data all_monster[MAX_CLIENT]){
    int id = data->id;
    if(data->state == DISCONNECT){
        clear_place(all_pac[id].pos[0], all_pac[id].pos[1]);
        clear_place(all_monster[id].pos[0], all_monster[id].pos[1]);                
    }
    else if(data->state == CHANGE){
        if(data->type == PACMAN){
            paint_pacman(all_pac[id].pos[0], all_pac[id].pos[1], all_pac[id].color[0], all_pac[id].color[1], all_pac[id].color[2]);                          
        }
        else if(data->type == MONSTER){
            paint_monster(all_monster[id].pos[0], all_monster[id].pos[1], all_monster[id].color[0], all_monster[id].color[1], all_monster[id].color[2]);
        }
    }           
    else if(data->type == PACMAN){ //pacman   
        clear_place(previous->pos[0], previous->pos[1]);                 
        paint_pacman(all_pac[id].pos[0], all_pac[id].pos[1], all_pac[id].color[0], all_pac[id].color[1], all_pac[id].color[2]);                          
    }
    else if(data->type == MONSTER){
        clear_place(previous->pos[0], previous->pos[1]);
        paint_monster(all_monster[id].pos[0], all_monster[id].pos[1], all_monster[id].color[0], all_monster[id].color[1], all_monster[id].color[2]);
    }
    else if(data->type == FRUIT){
        if(data->state == ERASE_FRUIT){
            clear_place(data->pos[0], data->pos[1]);
        }
        else{
            paint_lemon(data->pos[0], data->pos[1]);
        }
    }
    else if(data->type == POWER_PACMAN){
        clear_place(previous->pos[0], previous->pos[1]);                 
        paint_powerpacman(all_pac[id].pos[0], all_pac[id].pos[1], all_pac[id].color[0], all_pac[id].color[1], all_pac[id].color[2]);
    }
}